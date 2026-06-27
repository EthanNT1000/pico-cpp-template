# Bitwise Tricks in Embedded C/C++

Bitwise operations replace expensive instructions (division, modulo, branching)
with single-cycle operations. On a Cortex-M0+ (RP2040) there is no hardware
divider, so avoiding division matters even more than on desktop.

---

## Alignment — `(size + 7) & ~7`

Rounds `size` **up** to the next multiple of 8.

### Why it works

`~7` in binary (32-bit):
```
 7  = 0000 0000 0000 0111
~7  = 1111 1111 1111 1000   ← zeros the bottom 3 bits
```

ANDing any number with `~7` **clears** the bottom 3 bits, rounding **down**
to a multiple of 8. Adding 7 first ensures we round **up** instead:

| `size` | `size + 7` | `& ~7` | result |
|--------|-----------|--------|--------|
| 1      | 8         | 8      | 8      |
| 7      | 14        | 8      | 8      |
| 8      | 15        | 8      | 8  ← already aligned, stays |
| 9      | 16        | 16     | 16     |
| 15     | 22        | 16     | 16     |
| 16     | 23        | 16     | 16     |

### General form

```cpp
// Round x UP to next multiple of align (align must be a power of 2)
x = (x + (align - 1)) & ~(align - 1);

// Common variants:
(x + 3)  & ~3   // align to 4 bytes (uint32_t)
(x + 7)  & ~7   // align to 8 bytes (uint64_t / double)
(x + 15) & ~15  // align to 16 bytes (SIMD / cache line start)
(x + 31) & ~31  // align to 32 bytes (cache line)
```

### Why alignment matters on ARM

The RP2040 Cortex-M0+ does **not** support unaligned 32-bit or 64-bit
memory access — it hard faults. The STM32H7 Cortex-M7 supports unaligned
access but it takes extra cycles. Always align heap allocations to at least 8
bytes to cover every primitive type safely.

### Round DOWN (truncate to alignment boundary)

```cpp
x = x & ~(align - 1);   // no +offset needed
```

---

## Odd / even — `a & 1` instead of `a % 2`

```cpp
if (a % 2)  { }   // requires division — slow on M0+
if (a & 1)  { }   // single AND instruction — always use this
```

The LSB (bit 0) of any integer is 1 for odd numbers, 0 for even.
No division involved.

```
5 = 0101  →  5 & 1 = 1  (odd)
4 = 0100  →  4 & 1 = 0  (even)
```

---

## Modulo by power of 2 — `x & (n - 1)` instead of `x % n`

Works **only** when `n` is a power of 2.

```cpp
x % 8    // division
x & 7    // AND  — same result when n is power of 2

x % 256  // division
x & 255  // AND
```

Common use — wrap a ring buffer index without branching:

```cpp
head = (head + 1) & (BUFFER_SIZE - 1);  // BUFFER_SIZE must be power of 2
```

---

## Power-of-2 check — `(n & (n - 1)) == 0`

```cpp
bool is_power_of_2 = (n != 0) && ((n & (n - 1)) == 0);
```

If `n` is a power of 2, it has exactly one bit set. Subtracting 1 flips all
lower bits and clears that one bit — ANDing gives zero:

```
8  = 1000
7  = 0111
8 & 7 = 0000  ✓ power of 2

6  = 0110
5  = 0101
6 & 5 = 0100  ✗ not power of 2
```

Use this to assert alignment requirements at compile time:

```cpp
static_assert((BUFFER_SIZE & (BUFFER_SIZE - 1)) == 0,
    "BUFFER_SIZE must be a power of 2");
```

---

## Bit read / set / clear / toggle

```cpp
uint32_t reg = SOME_REGISTER;

// Check if bit N is set
bool set = (reg >> N) & 1;
bool set = reg & (1u << N);    // equivalent

// Set bit N
reg |= (1u << N);

// Clear bit N
reg &= ~(1u << N);

// Toggle bit N
reg ^= (1u << N);
```

These are the building blocks of every bare-metal peripheral driver.
`1u` (unsigned) avoids undefined behaviour when shifting into the sign bit.

---

## Multiply and divide by power of 2

```cpp
x * 4   →   x << 2
x / 4   →   x >> 2   // unsigned only; signed right-shift is implementation-defined
x * 8   →   x << 3
```

Modern compilers do this automatically, but knowing it helps when reading
disassembly or writing time-critical ISR code.

---

## Isolate the lowest set bit — `n & (-n)`

```cpp
uint32_t lowest = n & (-n);
```

```
n      = 0b 1010 1100
-n     = 0b 0101 0100   (two's complement)
n & -n = 0b 0000 0100   ← only the lowest set bit
```

Used in priority schedulers (including FreeRTOS internals) to find the
highest-priority ready task in one instruction.

---

## Clear the lowest set bit — `n & (n - 1)`

```cpp
n = n & (n - 1);
```

Each call removes exactly one set bit. Loop until `n == 0` to count or
process every set bit:

```cpp
while (n) {
    process(n & -n);   // handle lowest set bit
    n = n & (n - 1);   // remove it
}
```

---

## Quick reference

| Goal | Expression | Notes |
|---|---|---|
| Round up to align (pow2) | `(x + align - 1) & ~(align - 1)` | align must be power of 2 |
| Round down to align (pow2) | `x & ~(align - 1)` | |
| Is odd | `x & 1` | replaces `x % 2` |
| Modulo power of 2 | `x & (n - 1)` | replaces `x % n`, n must be power of 2 |
| Is power of 2 | `n && !(n & (n - 1))` | |
| Check bit N | `(x >> N) & 1` | |
| Set bit N | `x \| (1u << N)` | |
| Clear bit N | `x & ~(1u << N)` | |
| Toggle bit N | `x ^ (1u << N)` | |
| Lowest set bit | `n & (-n)` | |
| Clear lowest set bit | `n & (n - 1)` | |
| Multiply by 2^k | `x << k` | |
| Divide by 2^k | `x >> k` | unsigned only |
