# Section Garbage Collection in Embedded Systems

## What Is It?

When the compiler builds your code, it normally groups all functions and global
variables from a single `.cpp`/`.c` file into a few large sections (`.text`,
`.data`, `.bss`). The linker then includes or excludes those sections as a
whole — so if you use *one* function from a file, every other function in that
file comes along for the ride.

**Section garbage collection** breaks that coupling:

| Flag | Effect |
|---|---|
| `-ffunction-sections` | Each function gets its own section, e.g. `.text.myFunc` |
| `-fdata-sections` | Each global/static variable gets its own section, e.g. `.data.myVar` |
| `--gc-sections` (linker) | The linker removes every section with no live references |

The result: only code and data that is actually reachable from your entry point
ends up in the final binary.

## Why It Matters on a Pico

The RP2040/RP2350 has 2 MB of flash. Libraries like **micro-ROS** and
**FreeRTOS** contain many features you may never call. Without section GC a
simple micro-ROS "hello world" can easily exceed flash by pulling in the entire
library.

With section GC the linker strips every unused function and variable, keeping
the binary as small as possible.

## How the Pico SDK Wires This Up

The pico-sdk CMake toolchain already passes `--gc-sections` to the linker for
all Pico targets. You still need to compile *your* code (and any libraries you
build from source) with the matching compiler flags so the linker has individual
sections to work with:

```cmake
add_compile_options(-ffunction-sections -fdata-sections)
```

This goes in `CMakeLists.txt` **before** `add_executable`, so it applies to
all compiled sources including FreeRTOS and your application files.

> **Note:** micro-ROS is shipped as a pre-compiled static library
> (`libmicroros.a`), so these flags do not recompile it. The linker's
> `--gc-sections` still strips unused object-file sections *from inside* the
> archive, which is where the real savings come from.

## Things That Must Survive GC

The linker can only keep code it can trace from a live root. A few patterns
need extra protection:

### Interrupt handlers

Handlers referenced only through the vector table (not called directly in C)
must be marked so the linker does not discard them:

```c
void __attribute__((used)) isr_uart0(void) { ... }
```

The pico-sdk linker scripts already wrap the default vector table entries in
`KEEP()`, so SDK-registered IRQ handlers are safe without the attribute.

### C++ global constructors / destructors

The pico-sdk linker script keeps `.init_array` and `.fini_array` with
`KEEP()`, so global constructors and destructors run correctly even with
`--gc-sections`.

### Deliberately unused symbols (e.g. debug tables)

```c
const char build_info[] __attribute__((used, section(".rodata"))) = "v1.2.3";
```

## Quick Checklist

- [ ] `add_compile_options(-ffunction-sections -fdata-sections)` in `CMakeLists.txt`
- [ ] pico-sdk version ≥ 1.5 (ships `--gc-sections` by default)
- [ ] IRQ handlers registered via `irq_set_exclusive_handler` / `pico_sdk` macros (handled automatically)
- [ ] Any hand-rolled vector-table entries marked `__attribute__((used))`

## Further Reading

- [GCC manual — `-ffunction-sections`](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [LD manual — `--gc-sections`](https://sourceware.org/binutils/docs/ld/Options.html)
- [pico-sdk CMake config](https://github.com/raspberrypi/pico-sdk/blob/master/cmake/preload/toolchains/pico_arm_gcc.cmake)
- [micro-ROS on Pico](https://github.com/micro-ROS/micro_ros_raspberrypi_pico_sdk)
