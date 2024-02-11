#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef  int8_t s1;
typedef uint8_t u1;

const u1      u1_max =  255u;
const s1      s1_max =  127;
const u1      u1_min =    0u;
const s1      s1_min = -128;
const uint8_t s1_shf =    8u;
const uint8_t s1_msk =  u1_max;

typedef struct {
    s1 hi;
    u1 lo;
} s2;

const s2 s2_max = { .hi =  127, .lo = 255u };
const s2 s2_min = { .hi = -128, .lo = 255u };

bool isNeg(const s2 a) {
    return a.hi < 0;
}


// -1, 0, 1
int s2_cmp(const s2 a, const s2 b) {
    if (a.hi != b.hi) {
        // high order parts are unequal
        return a.hi < b.hi ? -1 : 1;
    }

    // high order parts are equal

    if (a.lo == b.lo) {
        return 0;
    }

    int ret = a.lo < b.lo ? -1 : 1;
    return ret;
}

s2 s2_from_int16(const int16_t a) {
    const s2 ret =  {.hi = a >> s1_shf, .lo = a & s1_msk};
    return ret;
}

int16_t int16_from_s2(const s2 a) {
    int16_t ret = (int16_t)(uint16_t)(a.hi << s1_shf | a.lo);
    return ret;
}

s2 s2_add(const s2 a, const s2 b) {
    s2 ret;
    s1 carry = (s1)(a.lo > u1_max - b.lo);
    ret.lo = a.lo + b.lo;
    ret.hi = a.hi + b.hi + carry;
    return ret;
}

s2 s2_sub(const s2 a, const s2 b) {
    s2 ret;
    s1 borrow = (s1)(a.lo < u1_min + b.lo);
    ret.lo = a.lo - b.lo;
    ret.hi = a.hi - b.hi - borrow;
    return ret;
}

//--------------------------------------------

const int _format_func_width = 15;

void test_conversion(void) {
    printf("%-*s()..  ", _format_func_width, __func__);
    int16_t p = INT16_MIN;
    do {
        const s2      a = s2_from_int16(p);
        const int16_t b = int16_from_s2(a);
        if (b != p) {
            printf("uh oh: %d -> %d\n", p, b);
            return;
        }
        p += 1;
    } while (p < INT16_MAX);

    printf("passed.\n");
}

#define ARRAYSIZE(a) ( sizeof(a) / sizeof(*a) )

const int16_t fun_values[] = {
                     0,
                     1,
                    -1,
        INT16_MIN     ,
        INT16_MIN +  1,
        INT16_MAX -  1,
        INT16_MAX     ,
        12345         ,
        1234          ,
        -4321         ,
        -12345        ,
    };

void test_add(void) {
    printf("%-*s()..  ", _format_func_width, __func__);

    for (int n = 0; n < ARRAYSIZE(fun_values); ++n) {
        int16_t p = fun_values[n];
        s2 a = s2_from_int16(p);
        for (int m = 0; m < ARRAYSIZE(fun_values); ++m) {
            int16_t q     = fun_values[m];
            s2      b     = s2_from_int16(q);
            s2      c     = s2_add(a, b);
            int16_t op_ab = int16_from_s2(c);
            int16_t op_pq = p + q;

            if (op_ab != op_pq) {
                // do it again, for breakpointing:
                s2      c     = s2_add(a, b);
                int16_t op_ab = int16_from_s2(c);
                printf("uh oh: s2_add(%d, %d) -> %d, expected %d\n", p, q, op_ab, op_pq);
                return;
            }
        }
    }

    printf("passed.\n");
}

void test_sub(void) {
    printf("%-*s()..  ", _format_func_width, __func__);

    for (int n = 0; n < ARRAYSIZE(fun_values); ++n) {
        int16_t p = fun_values[n];
        s2 a = s2_from_int16(p);
        for (int m = 0; m < ARRAYSIZE(fun_values); ++m) {
            int16_t q     = fun_values[m];
            s2      b     = s2_from_int16(q);
            s2      c     = s2_sub(a, b);
            int16_t op_ab = int16_from_s2(c);
            int16_t op_pq = p - q;

            if (op_ab != op_pq) {
                // do it again, for breakpointing:
                s2      c     = s2_sub(a, b);
                int16_t op_ab = int16_from_s2(c);
                printf("uh oh: s2_sub(%d, %d) -> %d, expected %d\n", p, q, op_ab, op_pq);
                return;
            }
        }
    }

    printf("passed.\n");
}


void test_compare(void) {
    printf("%-*s()..  ", _format_func_width, __func__);

    for (int n = 0; n < ARRAYSIZE(fun_values); ++n) {
        int16_t p = fun_values[n];
        s2 a = s2_from_int16(p);
        for (int m = 0; m < ARRAYSIZE(fun_values); ++m) {
            int16_t q = fun_values[m];
            s2 b      = s2_from_int16(q);
            int op_ab = s2_cmp(a, b);
            int op_pq = p == q ? 0 : (p < q ? -1 : 1);
            
            if (op_ab != op_pq) {
                // do it again, for breakpointing:
                int op_ab = s2_cmp(a, b);
                printf("uh oh: s2_cmp(%d, %d) -> %d, expected %d\n", p, q, op_ab, op_pq);
                return;
            }
        }
    }

    printf("passed.\n");
}

void test(void) {
    test_conversion();
    test_compare();
    test_add();
    test_sub();
}

int main() {
    test();
}

