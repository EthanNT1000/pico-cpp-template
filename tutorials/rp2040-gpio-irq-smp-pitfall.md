# RP2040 GPIO IRQ + FreeRTOS SMP: The Cross-Core `gpio_set_irq_enabled` Pitfall

## The Setup

A common pattern for interrupt-driven peripherals on Pico is the **disable-in-ISR / re-enable-in-task** idiom for level-triggered GPIO interrupts:

```cpp
// ISR: disable so the level-sensitive signal doesn't flood the interrupt line
void irqHandler(uint gpio, uint32_t events) {
    gpio_set_irq_enabled(gpio, GPIO_IRQ_LEVEL_LOW, false);  // disable
    xSemaphoreGiveFromISR(_sem, &woken);
    portYIELD_FROM_ISR(woken);
}

// Task: re-enable after clearing the hardware condition
void serviceTask() {
    while (true) {
        xSemaphoreTake(_sem, portMAX_DELAY);
        clearHardwareFlag();
        gpio_set_irq_enabled(_pin, GPIO_IRQ_LEVEL_LOW, true);  // re-enable
    }
}
```

This works perfectly on single-core FreeRTOS. On **FreeRTOS SMP** (both RP2040 cores active), it silently breaks after the first interrupt.

---

## Why It Breaks

The RP2040 has **separate GPIO interrupt control registers per core**: `PROC0_INTE` and `PROC1_INTE`. The pico-sdk's `gpio_set_irq_enabled` writes to the register for whichever core is currently executing:

```c
// Inside pico-sdk hardware/gpio.c
io_irq_ctrl_hw_t *irq_ctrl_base = get_core_num()
    ? &iobank0_hw->proc1_irq_ctrl
    : &iobank0_hw->proc0_irq_ctrl;
```

When you call `gpio_set_irq_callback` or `gpio_set_irq_enabled_with_callback`, pico-sdk registers `IO_IRQ_BANK0` on the **calling core** (e.g. core 0). The ISR therefore always fires on core 0 and modifies **PROC0_INTE**.

Under FreeRTOS SMP, the service task can be scheduled on **either core**. If it runs on core 1, the re-enable call writes to **PROC1_INTE** — a completely different register. PROC0_INTE stays disabled forever.

```
Core 0                          Core 1
------                          ------
ISR fires → PROC0_INTE = 0     (not involved)
                                serviceTask → PROC0_INTE still 0
                                             PROC1_INTE = 1  ← wrong register
INT pin LOW, but no ISR fires on core 0
```

### Symptom

- The interrupt fires exactly **once**.
- After that, the hardware INT pin stays LOW (confirmed by `gpio_get`), CANINTF/status registers show pending flags, but the ISR never fires again.
- No crash, no assert — just silent deadlock.

---

## The Fix: Use Edge-Triggered Interrupts

Switch from `GPIO_IRQ_LEVEL_LOW` to `GPIO_IRQ_EDGE_FALL`. With edge detection, there is nothing to enable or disable from task context — the pico-sdk clears the edge latch automatically inside its GPIO IRQ handler before calling your callback, so the next falling edge always re-arms the interrupt regardless of which core processes it.

```cpp
// Register edge-fall instead of level-low
gpioRegisterIrqCallback(_pin, GPIO_IRQ_EDGE_FALL,
    [this](uint gpio, uint32_t events){ irqHandler(gpio, events); });

// ISR: no gpio_set_irq_enabled needed
void irqHandler(uint gpio, uint32_t events) {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(_sem, &woken);
    portYIELD_FROM_ISR(woken);
}
```

### Handling the Burst Case

With edge-triggered interrupts, a second interrupt flag that fires while INT is **already LOW** produces no new falling edge, so no second ISR call. After clearing the flags you handled, check the pin directly:

```cpp
void serviceTask() {
    while (true) {
        xSemaphoreTake(_sem, portMAX_DELAY);

        uint8_t flags = readHardwareFlags();
        clearHardwareFlags(flags);

        // If INT is still LOW, another flag fired while we were processing.
        // No falling edge will occur (pin was already LOW), so kick manually.
        if (!gpio_get(_pin)) {
            xSemaphoreGive(_sem);
        }

        dispatchToConsumers(flags);
    }
}
```

This is safe across both cores: `gpio_get` reads a shared input register that is the same from either core. No per-core register writes happen in the task at all.

---

## Alternatives (and Why They Still Break)

| Approach | Problem |
|---|---|
| `irq_set_enabled(IO_IRQ_BANK0, false/true)` | Same issue — operates on the current core's NVIC |
| Pin the service task to core 0 with `vTaskCoreAffinitySet` | Works, but ties peripheral handling to one core and is easy to forget |
| Use `GPIO_IRQ_LEVEL_LOW` with re-enable inside the ISR | Floods the interrupt line before the hardware flag is cleared |

The edge-triggered + manual re-trigger pattern avoids all of these.

---

## Quick Reference

```
gpio_set_irq_enabled   → core-specific (PROC0_INTE or PROC1_INTE)
irq_set_enabled        → core-specific (NVIC enable for current core)
gpio_get               → shared (reads pad input, same from any core)
gpio_set_irq_callback  → registers ISR on the calling core
```

Only use `gpio_set_irq_enabled` / `irq_set_enabled` from the **same core** that originally set up the interrupt. In FreeRTOS SMP, avoid calling them from task context at all unless the task is pinned to the correct core.
