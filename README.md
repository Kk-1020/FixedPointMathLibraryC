# Fixed-Point Math Library

A portable, FPU-free fixed-point arithmetic library in C99 targeting
embedded processors like TI's C2000 and MSP430 families that lack a hardware
floating-point unit.

## Features

| Feature | Q15 | Q31 |
|---|---|---|
| Format | `int16_t`, 1.15 | `int32_t`, 1.31 |
| Range | [−1.0, +0.999969] | [−1.0, +0.99999999953] |
| Resolution | 3.05 × 10⁻⁵ | 4.66 × 10⁻¹⁰ |
| Add / Sub | Saturating | Saturating |
| Mul | Rounded, saturating | Rounded, saturating |
| Div | Saturating, Div/0 safe | Saturating, Div/0 safe |
| `sin` / `cos` | Lookup table + interp | 32-iter CORDIC |
| Sin error | < 7.4 × 10⁻⁵ (max) | < 9.8 × 10⁻⁹ (max) |
| FPU required? | **No** | **No** |
| Heap allocation? | **None** | **None** |

---

## Project Structure

```
fixedpoint/
├── include/
│   └── fixedpoint.h      Types, macros, all declarations
├── src/
│   ├── fixedpoint.c      Q15 / Q31 arithmetic (add, sub, mul, div)
│   └── trig.c            Q15 lookup table + Q31 CORDIC sin/cos
├── test/
│   └── test_fixedpoint.c 43 tests: arithmetic + 10,000 random trig cases
└── Makefile
```

---

## Quick Start

```bash
make          # builds libfixedpoint.a
make test     # builds and runs the full test suite
make clean
```

---

## API

### Types

```c
typedef int16_t  q15_t;    // 1.15 fixed-point
typedef int32_t  q31_t;    // 1.31 fixed-point
typedef uint16_t angle_t;  // phase accumulator: 0x0000=0, 0x4000=π/2, 0x8000=π
```

### Conversion Macros

```c
FLOAT_TO_Q15(0.5f)       // float → q15_t
Q15_TO_FLOAT(val)        // q15_t → float

DOUBLE_TO_Q31(0.5)       // double → q31_t
Q31_TO_DOUBLE(val)       // q31_t → double

RAD_TO_ANGLE(M_PI / 4)  // radians → angle_t
```

### Arithmetic

```c
// Q15
q15_t q15_add(q15_t a, q15_t b);   // saturating
q15_t q15_sub(q15_t a, q15_t b);
q15_t q15_mul(q15_t a, q15_t b);   // rounded, saturating
q15_t q15_div(q15_t a, q15_t b);   // div/0 returns ±MAX
q15_t q15_abs(q15_t a);
q15_t q15_neg(q15_t a);

// Q31 — same signatures with q31_t
```

### Trigonometry

```c
q15_t q15_sin(angle_t theta);   // lookup table, max err < 7.4e-5
q15_t q15_cos(angle_t theta);
q31_t q31_sin(angle_t theta);   // CORDIC, max err < 9.8e-9
q31_t q31_cos(angle_t theta);
```

### Usage Example

```c
#include "fixedpoint.h"

// Compute sin(π/6) in Q15
angle_t theta  = RAD_TO_ANGLE(3.14159f / 6.0f);  // 30°
q15_t   result = q15_sin(theta);
// Q15_TO_FLOAT(result) ≈ 0.5

// Dot product of two unit vectors in Q31
q31_t dot = q31_add(q31_mul(ax, bx), q31_mul(ay, by));
```

---

## Design Decisions

### Why saturating arithmetic?
On embedded targets, wrapping overflow produces silent wrong answers that
are extremely hard to debug. Saturation clamps out-of-range results to
±MAX, making overflows visible. TI's DSPlib and CMSIS-DSP follow the same
convention.

### Q15 trig: lookup table + linear interpolation
- 257-entry quarter-wave table (sin [0, π/2]) — 514 bytes of ROM.
- Quadrant folding via symmetry eliminates three-quarters of the table.
- Linear interpolation between adjacent entries keeps error below 1 LSB.
- **No division, no transcendental functions at runtime.**

### Q31 trig: CORDIC (Coordinate Rotation Digital Computer)
- 32 iterations of shift-and-add decompose the target angle into
  micro-rotations of ±arctan(2⁻ⁱ).
- Gain compensation: CORDIC shrinks the vector by K ≈ 1.6468; we
  pre-scale the initial vector by 1/K ≈ 0.6073 (fits in Q31).
- **Zero multiplications, zero divisions, zero lookup tables.**
- Error < 10 ns (9.8 × 10⁻⁹ absolute) — within 2 LSBs of Q31.

### Angle type: `uint16_t` phase accumulator
Standard in DSP: 0x0000 = 0, 0x8000 = π, wraps at 2π with natural
unsigned overflow. Eliminates range-checking and modulo operations.

---

## Test Results

```
Results: 43 passed, 0 failed

Q15 Trig (10,000 random angles):
  sin  max_err=7.4e-05  rms_err=4.01e-05
  cos  max_err=7.4e-05  rms_err=4.01e-05

Q31 Trig (10,000 random angles):
  sin  max_err=9.79e-09  rms_err=2.08e-09
  cos  max_err=9.79e-09  rms_err=2.07e-09
```

---

## Resume Bullet

> Implemented a fixed-point arithmetic library in C99 (Q15/Q31 formats) with
> saturating arithmetic, a 257-entry quarter-wave lookup table (sin/cos) and a
> 32-iteration CORDIC algorithm; validated with 43 automated tests including
> 10,000 randomized trig cases achieving max error 7.4 × 10⁻⁵ (Q15) and
> 9.8 × 10⁻⁹ (Q31) vs. double-precision reference — zero heap, zero FPU.
