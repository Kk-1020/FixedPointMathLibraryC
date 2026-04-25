/**
 * fixedpoint.c — Q15 / Q31 arithmetic
 *
 * All operations are saturating (clamp on overflow rather than wrap).
 * No heap allocation, no floating-point at runtime.
 */

#include "fixedpoint.h"

/* =========================================================================
 * Q15 Arithmetic
 * ========================================================================= */

q15_t q15_add(q15_t a, q15_t b)
{
    return q15_sat((int32_t)a + (int32_t)b);
}

q15_t q15_sub(q15_t a, q15_t b)
{
    return q15_sat((int32_t)a - (int32_t)b);
}

/*
 * Q15 multiplication:
 *   Full product is 32-bit (s.30 format), shift right 15 to get s.15.
 *   We add 0x4000 before shifting to round to nearest instead of truncating.
 */
q15_t q15_mul(q15_t a, q15_t b)
{
    int32_t prod = (int32_t)a * (int32_t)b;
    prod = (prod + 0x4000) >> 15;
    return q15_sat(prod);
}

/*
 * Q15 division:
 *   Conceptually: result = a / b  (both in Q15)
 *   In integer: result = (a << 15) / b  (produces Q15 quotient)
 *   We use int32 to hold the shifted numerator without overflow
 *   since |a| < 2^15 and we shift left 15: max numerator = 2^29.
 */
q15_t q15_div(q15_t a, q15_t b)
{
    if (b == 0) {
        return (a >= 0) ? Q15_MAX : Q15_MIN;
    }
    int32_t result = ((int32_t)a << 15) / (int32_t)b;
    return q15_sat(result);
}

q15_t q15_abs(q15_t a)
{
    if (a == Q15_MIN) return Q15_MAX;   /* saturate: |-1.0| → 0.999969 */
    return (a < 0) ? (q15_t)(-a) : a;
}

q15_t q15_neg(q15_t a)
{
    if (a == Q15_MIN) return Q15_MAX;
    return (q15_t)(-a);
}

/* =========================================================================
 * Q31 Arithmetic
 * ========================================================================= */

q31_t q31_add(q31_t a, q31_t b)
{
    return q31_sat((int64_t)a + (int64_t)b);
}

q31_t q31_sub(q31_t a, q31_t b)
{
    return q31_sat((int64_t)a - (int64_t)b);
}

/*
 * Q31 multiplication:
 *   Full product is 64-bit (s.62 format), shift right 31 → s.31.
 *   Rounding: add 0x40000000 (= 2^30) before shift.
 *
 *   Special case: Q31_MIN × Q31_MIN = 2^62, which overflows int64_t
 *   (max = 2^63 - 1), but after >>31 gives 2^31 which saturates to Q31_MAX.
 */
q31_t q31_mul(q31_t a, q31_t b)
{
    int64_t prod = (int64_t)a * (int64_t)b;
    prod = (prod + 0x40000000LL) >> 31;
    return q31_sat(prod);
}

/*
 * Q31 division:
 *   result_q31 = (a_q31 / b_q31)
 *   Integer form: (a << 31) / b
 *   |a| < 2^31, shift 31 → needs 62-bit numerator → use int64_t.
 */
q31_t q31_div(q31_t a, q31_t b)
{
    if (b == 0) {
        return (a >= 0) ? Q31_MAX : Q31_MIN;
    }
    int64_t result = ((int64_t)a << 31) / (int64_t)b;
    return q31_sat(result);
}

q31_t q31_abs(q31_t a)
{
    if (a == Q31_MIN) return Q31_MAX;
    return (a < 0) ? (q31_t)(-a) : a;
}

q31_t q31_neg(q31_t a)
{
    if (a == Q31_MIN) return Q31_MAX;
    return (q31_t)(-a);
}
