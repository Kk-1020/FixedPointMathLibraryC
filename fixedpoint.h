/**
 * fixedpoint.h — Fixed-Point Arithmetic Library
 *
 * Supports Q15 (1.15) and Q31 (1.31) formats.
 *
 * Q15  : int16_t, 1 sign bit + 15 fractional bits
 *        Range  : [-1.0,  +0.999969]   LSB ≈ 3.05e-5
 *
 * Q31  : int32_t, 1 sign bit + 31 fractional bits
 *        Range  : [-1.0,  +0.999999999]  LSB ≈ 4.66e-10
 *
 * Trigonometry:
 *   Q15 sin/cos — 256-entry quarter-wave lookup table + linear interpolation
 *   Q31 sin/cos — 32-iteration CORDIC (no lookup, no FPU, no division)
 *
 * Angle type: angle_t (uint16_t phase accumulator)
 *   0x0000 = 0 rad | 0x4000 = π/2 | 0x8000 = π | 0xC000 = 3π/2
 *   Wraps naturally at 2π (0xFFFF + 1 = 0x0000)
 *
 * Target: bare-metal C (C99), no heap, no FPU, no stdio.
 */

#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H

#include <stdint.h>

/* =========================================================================
 * Types
 * ========================================================================= */

typedef int16_t  q15_t;
typedef int32_t  q31_t;
typedef uint16_t angle_t;   /* Phase accumulator — wraps at 2π */

/* =========================================================================
 * Constants
 * ========================================================================= */

#define Q15_MAX    ((q15_t) 0x7FFF)           /*  0.999969 */
#define Q15_MIN    ((q15_t)(int16_t)0x8000)   /* -1.0      */
#define Q15_HALF   ((q15_t) 0x4000)           /*  0.5      */
#define Q15_SCALE  32768.0f

#define Q31_MAX    ((q31_t) 0x7FFFFFFF)
#define Q31_MIN    ((q31_t)(int32_t)0x80000000)
#define Q31_HALF   ((q31_t) 0x40000000)
#define Q31_SCALE  2147483648.0

/* =========================================================================
 * Conversion Macros
 * ========================================================================= */

#define FLOAT_TO_Q15(x)    ((q15_t)((float)(x)  * Q15_SCALE))
#define Q15_TO_FLOAT(x)    ((float)(x)  / Q15_SCALE)

#define DOUBLE_TO_Q31(x)   ((q31_t)((double)(x) * Q31_SCALE))
#define Q31_TO_DOUBLE(x)   ((double)(x) / Q31_SCALE)

/* angle_t: radians [0, 2π) → uint16_t [0, 65536)               */
#define RAD_TO_ANGLE(r)    ((angle_t)((float)(r) / (2.0f * 3.14159265358979f) * 65536.0f))
#define ANGLE_TO_RAD(a)    ((double)(a)           / 65536.0 * (2.0 * 3.14159265358979))

/* =========================================================================
 * Inline Saturation Helpers
 * ========================================================================= */

static inline q15_t q15_sat(int32_t x)
{
    if (x >  (int32_t)Q15_MAX) return Q15_MAX;
    if (x < -(int32_t)32768)   return Q15_MIN;
    return (q15_t)x;
}

static inline q31_t q31_sat(int64_t x)
{
    if (x >  (int64_t)Q31_MAX)          return Q31_MAX;
    if (x < -(int64_t)2147483648LL - 1) return Q31_MIN;
    return (q31_t)x;
}

/* =========================================================================
 * Q15 Arithmetic
 * ========================================================================= */

/** Saturating addition: a + b, clamped to [Q15_MIN, Q15_MAX] */
q15_t q15_add(q15_t a, q15_t b);

/** Saturating subtraction: a - b */
q15_t q15_sub(q15_t a, q15_t b);

/** Q15 × Q15 → Q15 (rounded, saturating) */
q15_t q15_mul(q15_t a, q15_t b);

/** Q15 ÷ Q15 → Q15 (saturating; returns MAX/MIN on divide-by-zero) */
q15_t q15_div(q15_t a, q15_t b);

/** Saturating absolute value */
q15_t q15_abs(q15_t a);

/** Saturating negation */
q15_t q15_neg(q15_t a);

/* =========================================================================
 * Q31 Arithmetic
 * ========================================================================= */

q31_t q31_add(q31_t a, q31_t b);
q31_t q31_sub(q31_t a, q31_t b);
q31_t q31_mul(q31_t a, q31_t b);
q31_t q31_div(q31_t a, q31_t b);
q31_t q31_abs(q31_t a);
q31_t q31_neg(q31_t a);

/* =========================================================================
 * Trigonometry
 * ========================================================================= */

/**
 * Q15 sine — lookup table with linear interpolation.
 * @param theta  Phase angle (angle_t): 0x0000=0, 0x4000=π/2, 0x8000=π …
 * @return       sin(theta) in Q15 format
 */
q15_t q15_sin(angle_t theta);

/** Q15 cosine — implemented as sin(theta + π/2) */
q15_t q15_cos(angle_t theta);

/**
 * Q31 sine — 32-iteration CORDIC (no division, no FPU, no table).
 * Error < 2 LSBs (< 1e-9 absolute).
 */
q31_t q31_sin(angle_t theta);

/** Q31 cosine — computed simultaneously with sin via CORDIC */
q31_t q31_cos(angle_t theta);

#endif /* FIXEDPOINT_H */
