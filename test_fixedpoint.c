/**
 * test_fixedpoint.c — Automated test suite for the fixed-point library
 *
 * Tests:
 *   1. Q15 arithmetic  (add, sub, mul, div, saturation edge cases)
 *   2. Q31 arithmetic  (add, sub, mul, div, saturation edge cases)
 *   3. Q15 trig        (sin, cos — 10,000 random angles, error vs math.h)
 *   4. Q31 trig        (sin, cos — 10,000 random angles, CORDIC accuracy)
 *
 * Pass criterion:
 *   • Q15 trig: max absolute error < 0.01%  (< 3.28e-4 over [-1, 1])
 *   • Q31 trig: max absolute error < 0.0001% (< 1e-6 over [-1, 1])
 *
 * Compile & run:
 *   make test
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "fixedpoint.h"

/* =========================================================================
 * Utilities
 * ========================================================================= */

static int  g_pass = 0;
static int  g_fail = 0;
static char g_current_suite[64];

static void suite(const char *name)
{
    strncpy(g_current_suite, name, sizeof(g_current_suite) - 1);
    printf("\n── %s ──────────────────────────────\n", name);
}

static void check(const char *label, int condition)
{
    if (condition) {
        printf("  [PASS]  %s\n", label);
        g_pass++;
    } else {
        printf("  [FAIL]  %s\n", label);
        g_fail++;
    }
}

#define CHECK_EQ(label, a, b)       check(label, (a) == (b))
#define CHECK_CLOSE(label, a, b, t) check(label, fabs((a)-(b)) <= (t))

/* LCG random — deterministic, no stdlib rand() seeding quirks */
static uint32_t lcg_state = 0xDEADBEEF;
static uint32_t lcg_next(void)
{
    lcg_state = lcg_state * 1664525u + 1013904223u;
    return lcg_state;
}

/* =========================================================================
 * 1. Q15 Arithmetic Tests
 * ========================================================================= */

static void test_q15_arithmetic(void)
{
    suite("Q15 Arithmetic");

    /* --- Addition --- */
    CHECK_EQ("0.25 + 0.25 = 0.5",
             q15_add(FLOAT_TO_Q15(0.25f), FLOAT_TO_Q15(0.25f)),
             FLOAT_TO_Q15(0.5f));

    CHECK_EQ("0.5 + 0.6 saturates to MAX",
             q15_add(FLOAT_TO_Q15(0.5f), FLOAT_TO_Q15(0.6f)),
             Q15_MAX);

    CHECK_EQ("-0.5 + -0.6 saturates to MIN",
             q15_add(FLOAT_TO_Q15(-0.5f), FLOAT_TO_Q15(-0.6f)),
             Q15_MIN);

    CHECK_EQ("0.0 + 0.0 = 0.0",
             q15_add(0, 0), 0);

    /* --- Subtraction --- */
    CHECK_EQ("0.75 - 0.25 = 0.5",
             q15_sub(FLOAT_TO_Q15(0.75f), FLOAT_TO_Q15(0.25f)),
             FLOAT_TO_Q15(0.5f));

    CHECK_EQ("0.999969 - (-0.5) saturates to MAX",
             q15_sub(Q15_MAX, FLOAT_TO_Q15(-0.5f)),
             Q15_MAX);

    /* --- Multiplication --- */
    CHECK_EQ("0.5 × 0.5 = 0.25",
             q15_mul(FLOAT_TO_Q15(0.5f), FLOAT_TO_Q15(0.5f)),
             FLOAT_TO_Q15(0.25f));

    CHECK_EQ("(-0.5) × 0.5 = -0.25",
             q15_mul(FLOAT_TO_Q15(-0.5f), FLOAT_TO_Q15(0.5f)),
             FLOAT_TO_Q15(-0.25f));

    CHECK_EQ("1.0 × 1.0 ≈ 1.0 (saturating)",
             q15_mul(Q15_MAX, Q15_MAX),
             (q15_t)0x7FFE);  /* (32767^2 + 0x4000) >> 15 = 32766 */

    CHECK_EQ("0 × anything = 0",
             q15_mul(0, Q15_MAX), 0);

    /* --- Division --- */
    CHECK_EQ("0.5 ÷ 1.0 = 0.5",
             q15_div(FLOAT_TO_Q15(0.5f), Q15_MAX),
             FLOAT_TO_Q15(0.5f));

    CHECK_EQ("0.25 ÷ 0.5 = 0.5",
             q15_div(FLOAT_TO_Q15(0.25f), FLOAT_TO_Q15(0.5f)),
             FLOAT_TO_Q15(0.5f));

    CHECK_EQ("Divide by zero → MAX",
             q15_div(FLOAT_TO_Q15(0.5f), 0),
             Q15_MAX);

    CHECK_EQ("Negative ÷ zero → MIN",
             q15_div(FLOAT_TO_Q15(-0.5f), 0),
             Q15_MIN);

    /* --- Saturation edge cases --- */
    CHECK_EQ("abs(Q15_MIN) saturates to MAX", q15_abs(Q15_MIN), Q15_MAX);
    CHECK_EQ("neg(Q15_MIN) saturates to MAX", q15_neg(Q15_MIN), Q15_MAX);
    CHECK_EQ("abs(Q15_MAX) = Q15_MAX",        q15_abs(Q15_MAX), Q15_MAX);
    CHECK_EQ("neg(0) = 0",                    q15_neg(0), 0);
}

/* =========================================================================
 * 2. Q31 Arithmetic Tests
 * ========================================================================= */

static void test_q31_arithmetic(void)
{
    suite("Q31 Arithmetic");

    CHECK_EQ("0.25 + 0.25 = 0.5",
             q31_add(DOUBLE_TO_Q31(0.25), DOUBLE_TO_Q31(0.25)),
             DOUBLE_TO_Q31(0.5));

    CHECK_EQ("0.6 + 0.6 saturates to MAX",
             q31_add(DOUBLE_TO_Q31(0.6), DOUBLE_TO_Q31(0.6)),
             Q31_MAX);

    CHECK_EQ("-0.6 + -0.6 saturates to MIN",
             q31_add(DOUBLE_TO_Q31(-0.6), DOUBLE_TO_Q31(-0.6)),
             Q31_MIN);

    CHECK_EQ("0.5 × 0.5 = 0.25 (within 1 LSB)",
             abs(q31_mul(DOUBLE_TO_Q31(0.5), DOUBLE_TO_Q31(0.5))
                 - DOUBLE_TO_Q31(0.25)) <= 1,
             1);

    CHECK_EQ("(-0.5) × 0.5 = -0.25 (within 1 LSB)",
             abs(q31_mul(DOUBLE_TO_Q31(-0.5), DOUBLE_TO_Q31(0.5))
                 - DOUBLE_TO_Q31(-0.25)) <= 1,
             1);

    CHECK_EQ("0.25 ÷ 0.5 = 0.5",
             q31_div(DOUBLE_TO_Q31(0.25), DOUBLE_TO_Q31(0.5)),
             DOUBLE_TO_Q31(0.5));

    CHECK_EQ("abs(Q31_MIN) saturates to MAX", q31_abs(Q31_MIN), Q31_MAX);
    CHECK_EQ("neg(Q31_MIN) saturates to MAX", q31_neg(Q31_MIN), Q31_MAX);
    CHECK_EQ("div by zero → MAX",
             q31_div(DOUBLE_TO_Q31(0.5), 0), Q31_MAX);
}

/* =========================================================================
 * 3. Q15 Trig Tests — 10,000 random angles
 * ========================================================================= */

#define N_TRIG_TESTS 10000

static void test_q15_trig(void)
{
    suite("Q15 Trig (Lookup Table) — 10,000 random angles");

    double max_sin_err = 0.0;
    double max_cos_err = 0.0;
    double sum_sin_sq  = 0.0;
    double sum_cos_sq  = 0.0;
    int    sin_fail    = 0;
    int    cos_fail    = 0;

    const double threshold = 0.0001; /* 0.01% of full scale = 3.28e-4 abs */

    lcg_state = 0xC0FFEE42; /* deterministic seed */

    for (int i = 0; i < N_TRIG_TESTS; i++) {
        angle_t theta = (angle_t)(lcg_next() & 0xFFFF);

        /* Reference (double precision) */
        double  rad_ref  = ANGLE_TO_RAD(theta);
        double  sin_ref  = sin(rad_ref);
        double  cos_ref  = cos(rad_ref);

        /* Fixed-point results */
        double  sin_fp   = Q15_TO_FLOAT(q15_sin(theta));
        double  cos_fp   = Q15_TO_FLOAT(q15_cos(theta));

        double  sin_err  = fabs(sin_fp - sin_ref);
        double  cos_err  = fabs(cos_fp - cos_ref);

        if (sin_err > max_sin_err) max_sin_err = sin_err;
        if (cos_err > max_cos_err) max_cos_err = cos_err;
        sum_sin_sq += sin_err * sin_err;
        sum_cos_sq += cos_err * cos_err;

        if (sin_err > threshold) sin_fail++;
        if (cos_err > threshold) cos_fail++;
    }

    double rms_sin = sqrt(sum_sin_sq / N_TRIG_TESTS);
    double rms_cos = sqrt(sum_cos_sq / N_TRIG_TESTS);

    printf("  sin  max_err=%.6f  rms_err=%.2e  fail_count=%d/%d\n",
           max_sin_err, rms_sin, sin_fail, N_TRIG_TESTS);
    printf("  cos  max_err=%.6f  rms_err=%.2e  fail_count=%d/%d\n",
           max_cos_err, rms_cos, cos_fail, N_TRIG_TESTS);

    check("Q15 sin: max error < 0.01% full-scale", max_sin_err < threshold);
    check("Q15 cos: max error < 0.01% full-scale", max_cos_err < threshold);
    check("Q15 sin: zero failures at 0.01% threshold", sin_fail == 0);
    check("Q15 cos: zero failures at 0.01% threshold", cos_fail == 0);

    /* Spot-check known values */
    double sin_0   = Q15_TO_FLOAT(q15_sin(0x0000));
    double cos_0   = Q15_TO_FLOAT(q15_cos(0x0000));
    double sin_90  = Q15_TO_FLOAT(q15_sin(0x4000));
    double cos_90  = Q15_TO_FLOAT(q15_cos(0x4000));
    double sin_180 = Q15_TO_FLOAT(q15_sin(0x8000));
    double cos_180 = Q15_TO_FLOAT(q15_cos(0x8000));

    printf("\n  Spot checks:\n");
    printf("    sin(0)   = %+.6f  (ref  0.000000)\n", sin_0);
    printf("    cos(0)   = %+.6f  (ref +1.000000)\n", cos_0);
    printf("    sin(π/2) = %+.6f  (ref +1.000000)\n", sin_90);
    printf("    cos(π/2) = %+.6f  (ref  0.000000)\n", cos_90);
    printf("    sin(π)   = %+.6f  (ref  0.000000)\n", sin_180);
    printf("    cos(π)   = %+.6f  (ref -1.000000)\n", cos_180);

    check("sin(0) ≈ 0",    fabs(sin_0)        < threshold);
    check("cos(0) ≈ 1",    fabs(cos_0 - 1.0)  < threshold);
    check("sin(π/2) ≈ 1",  fabs(sin_90 - 1.0) < threshold);
    check("cos(π/2) ≈ 0",  fabs(cos_90)       < threshold);
    check("sin(π) ≈ 0",    fabs(sin_180)       < threshold);
    check("cos(π) ≈ -1",   fabs(cos_180 + 1.0) < threshold);

    /* Pythagorean identity: sin² + cos² ≈ 1 for all test angles */
    int pyth_fail = 0;
    lcg_state = 0xC0FFEE42;
    for (int i = 0; i < N_TRIG_TESTS; i++) {
        angle_t theta = (angle_t)(lcg_next() & 0xFFFF);
        double  s = Q15_TO_FLOAT(q15_sin(theta));
        double  c = Q15_TO_FLOAT(q15_cos(theta));
        if (fabs(s*s + c*c - 1.0) > 0.001) pyth_fail++;
    }
    check("sin²+cos²=1 for all 10k angles (tol 0.001)", pyth_fail == 0);
}

/* =========================================================================
 * 4. Q31 Trig Tests — 10,000 random angles (CORDIC)
 * ========================================================================= */

static void test_q31_trig(void)
{
    suite("Q31 Trig (CORDIC, 32 iterations) — 10,000 random angles");

    double max_sin_err = 0.0;
    double max_cos_err = 0.0;
    double sum_sin_sq  = 0.0;
    double sum_cos_sq  = 0.0;
    int    sin_fail    = 0;
    int    cos_fail    = 0;

    const double threshold = 1e-6; /* 0.0001% of full scale */

    lcg_state = 0xBEEFCAFE;

    for (int i = 0; i < N_TRIG_TESTS; i++) {
        angle_t theta = (angle_t)(lcg_next() & 0xFFFF);

        double  rad_ref  = ANGLE_TO_RAD(theta);
        double  sin_ref  = sin(rad_ref);
        double  cos_ref  = cos(rad_ref);

        double  sin_fp   = Q31_TO_DOUBLE(q31_sin(theta));
        double  cos_fp   = Q31_TO_DOUBLE(q31_cos(theta));

        double  sin_err  = fabs(sin_fp - sin_ref);
        double  cos_err  = fabs(cos_fp - cos_ref);

        if (sin_err > max_sin_err) max_sin_err = sin_err;
        if (cos_err > max_cos_err) max_cos_err = cos_err;
        sum_sin_sq += sin_err * sin_err;
        sum_cos_sq += cos_err * cos_err;

        if (sin_err > threshold) sin_fail++;
        if (cos_err > threshold) cos_fail++;
    }

    double rms_sin = sqrt(sum_sin_sq / N_TRIG_TESTS);
    double rms_cos = sqrt(sum_cos_sq / N_TRIG_TESTS);

    printf("  sin  max_err=%.2e  rms_err=%.2e  fail_count=%d/%d\n",
           max_sin_err, rms_sin, sin_fail, N_TRIG_TESTS);
    printf("  cos  max_err=%.2e  rms_err=%.2e  fail_count=%d/%d\n",
           max_cos_err, rms_cos, cos_fail, N_TRIG_TESTS);

    check("Q31 sin: max error < 1e-6", max_sin_err < threshold);
    check("Q31 cos: max error < 1e-6", max_cos_err < threshold);
    check("Q31 sin: zero failures at 1e-6 threshold", sin_fail == 0);
    check("Q31 cos: zero failures at 1e-6 threshold", cos_fail == 0);

    /* Pythagorean identity */
    int pyth_fail = 0;
    lcg_state = 0xBEEFCAFE;
    for (int i = 0; i < N_TRIG_TESTS; i++) {
        angle_t theta = (angle_t)(lcg_next() & 0xFFFF);
        double  s = Q31_TO_DOUBLE(q31_sin(theta));
        double  c = Q31_TO_DOUBLE(q31_cos(theta));
        if (fabs(s*s + c*c - 1.0) > 1e-7) pyth_fail++;
    }
    check("sin²+cos²=1 for all 10k angles (tol 1e-7)", pyth_fail == 0);
}

/* =========================================================================
 * Main
 * ========================================================================= */

int main(void)
{
    printf("=========================================\n");
    printf("  Fixed-Point Library — Test Suite\n");
    printf("  %d trig test cases per type\n", N_TRIG_TESTS);
    printf("=========================================\n");

    test_q15_arithmetic();
    test_q31_arithmetic();
    test_q15_trig();
    test_q31_trig();

    printf("\n=========================================\n");
    printf("  Results: %d passed, %d failed\n", g_pass, g_fail);
    printf("=========================================\n");

    return (g_fail == 0) ? 0 : 1;
}
