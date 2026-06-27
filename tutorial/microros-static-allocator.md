# micro-ROS Static Allocation on FreeRTOS

## Why Static Allocation

The core rule in embedded systems: **allocate at init, never at runtime, never free.**

Dynamic allocation at runtime causes:
- **Fragmentation** — Heap4 coalesces adjacent free blocks but not non-adjacent ones. Repeated alloc/free of different sizes eventually leaves enough total free RAM but no single block large enough.
- **Non-determinism** — `pvPortMalloc` time varies with heap state, breaking real-time guarantees.
- **Silent failure** — on a 264 KB device there is no recovery if you run out.

micro-ROS objects (node, publisher, subscriber, executor, timer) are created once at init and live for the full program lifetime — they are a natural fit for static allocation.

## The Bump Allocator

A bump allocator is a fixed buffer with a single head pointer that only moves forward. Allocation is O(1) and deterministic. There is no `free`.

```cpp
static uint8_t micro_ros_pool[8192];
static size_t  micro_ros_pool_head = 0;

static void* microros_allocate(size_t size, void* state) {
    (void)state;
    size = (size + 7) & ~7;  // 8-byte align
    if (micro_ros_pool_head + size > sizeof(micro_ros_pool)) {
        return nullptr;
    }
    void* ptr = micro_ros_pool + micro_ros_pool_head;
    micro_ros_pool_head += size;
    return ptr;
}

static void microros_deallocate(void* pointer, void* state) {
    (void)state;
    (void)pointer;
    // Intentional no-op: objects live for the program lifetime.
}

static void* microros_zero_allocate(size_t n, size_t size, void* state) {
    void* ptr = microros_allocate(n * size, state);
    if (ptr) memset(ptr, 0, n * size);
    return ptr;
}

static void* microros_reallocate(void* pv, size_t xWantedSize, void* state) {
    (void)pv;
    configASSERT(false && "reallocate called — increase executor handle count in rclc_executor_init");
    return nullptr;
}
```

Wire it into the `rcl_allocator_t` before calling any `rclc_*` init functions:

```cpp
rcl_allocator_t allocator  = rcutils_get_zero_initialized_allocator();
allocator.allocate         = microros_allocate;
allocator.deallocate       = microros_deallocate;
allocator.zero_allocate    = microros_zero_allocate;
allocator.reallocate       = microros_reallocate;

rclc_support_init(&support, 0, NULL, &allocator);
```

### Pool sizing

8 KB covers a typical node + 1–2 publishers + 1–2 subscribers + executor + timer.
If `microros_allocate` returns `nullptr` during init, increase the pool size.
A useful debug check after all `rclc_*` init calls:

```cpp
printf("micro-ROS pool used: %u / %u bytes\n",
    (unsigned)micro_ros_pool_head, (unsigned)sizeof(micro_ros_pool));
```

Tune the pool size down to leave as little headroom as you are comfortable with.

### FreeRTOS heap is untouched

The bump allocator only covers micro-ROS. Your FreeRTOS tasks, queues, and application code continue to use `pvPortMalloc` / `vPortFree` from Heap4. The two pools are fully independent.

## The Executor Handle Count Rule

`rclc_executor_init` takes an explicit handle count. Internally it allocates an array of exactly that size — no dynamic growth. If you add more handles than you reserved, micro-ROS calls `reallocate` to grow the array, which hits the `configASSERT` above.

**Rule: the handle count must equal the total number of `rclc_executor_add_*` calls.**

```cpp
// Handles added below:
//   rclc_executor_add_subscription  → 1
//   rclc_executor_add_timer         → 1
//                              total = 2
rclc_executor_init(&executor, &support.context, 2, &allocator);

rclc_executor_add_subscription(&executor, &subscriber, &msg, callback, ON_NEW_DATA);
rclc_executor_add_timer(&executor, &timer);
```

Each of these counts as one handle:

| Function | Count |
|---|---|
| `rclc_executor_add_subscription` | 1 |
| `rclc_executor_add_timer` | 1 |
| `rclc_executor_add_service` | 1 |
| `rclc_executor_add_action_server` | 1 |

If you add a second subscriber later and forget to update the count, the `configASSERT` in `microros_reallocate` fires at runtime with a message telling you exactly what to fix.

## Why `reallocate` Is a Stub

Standard `realloc` needs to copy the old data to the new buffer. To do that it needs the old allocation size. Heap4 and the bump allocator both discard that information after allocation, so a correct `reallocate` is not implementable without a custom size-tracking header.

The practical solution: keep the executor handle count accurate and `reallocate` is never called. The assert exists to enforce that invariant at runtime during development.
