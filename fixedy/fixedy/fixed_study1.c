#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define ARRAYSIZE(a) ( sizeof(a) / sizeof(*a) )

typedef  int8_t s1;
typedef uint8_t u1;

// todo: make all these a function of number of bits.
const u1      u1_max     =  255u;
const s1      s1_max     =  127;
const u1      u1_min     =    0u;
const s1      s1_min     = -128;
const uint8_t s1_shf     =    8u;
const uint8_t s1_shf_hlf =  s1_shf >> 1;
const uint8_t s1_msk_hlf =  0xf;
const uint8_t s1_msk     =  u1_max;


typedef struct {
    s1 hi;
    u1 lo;
} s2;

typedef struct {
    u1 hi;
    u1 lo;
} u2;

typedef struct {
    s2 hi;
    u2 lo;
} s4;

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

// out_result can be === a or b
void addWithCarry(const u1 a, const u1 b, u1 *out_result, u1 *out_carry) {
    *out_carry  += (u1)(a > u1_max - b);
    *out_result  = a + b;
}

// out_result can be === a or b
void subWithBorrow(const u1 a, const u1 b, u1 *out_result, u1 *out_borrow) {
    *out_borrow += (u1)(a < u1_min + b);
    *out_result  = a - b;
}

s2 s2_add(const s2 a, const s2 b) {
    s2 ret;
    u1 carry = 0;
    addWithCarry(a.lo, b.lo, &ret.lo, &carry);
    ret.hi = a.hi + b.hi + carry;
    return ret;
}

s2 s2_sub(const s2 a, const s2 b) {
    s2 ret;
    u1 borrow = 0;
    subWithBorrow(a.lo, b.lo, &ret.lo, &borrow);
    ret.hi = a.hi - b.hi - borrow;
    return ret;
}

u1 hi(const u1 n) {
    return n >> s1_shf_hlf;
}

u1 lo(const u1 n) {
    return n & s1_msk_hlf;
}

// nibble add with carry.
// a and b are added,
// the low nibble is stored in res,
// and the high nibble is added to the carry.
void nibAWC(const u1 a, const u1 b, u1 *res, u1 *carry) {
    *res    = a + b;
    *carry += hi(*res);
    *res    = lo(*res);
}

// same as nibAWC but without carry.
void nibAWOC(const u1 a, const u1 b, u1 *res) {
    *res    = lo(a + b);
}

s2 s2_mul(const s2 a, const s2 b) {
    
    // Simplified example where 'a' and 'b' are each single words.
    //
    //   AB  <-- a
    // x CD  <-- b
    // ----
    // = DB + DA< + CB< + CA<<
    //
    // AB is a word
    // where "<" = shifted one nibble. (so << = shifted one word)
    //
    //   +------------------------------------------------+
    //   | resulting nibbles. 0 is low-order, 3 is high.  |
    // = | 0. DB.lo                                       |
    //   | 1. DB.hi + DA.lo + CB.lo           -> carry#1  |
    //   | 2. DA.hi + CB.hi + CA.lo + carry#1 -> carry#2  |
    //   | 3. CA.hi                 + carry#2             |
    //   +------------------------------------------------+
    //
    //   .. but since our output here is a single word,
    //   only nibbles 0 and 1 are actually computed.
    //
    // In our actual code, 'a' and 'b' are each two-word compounds,
    // so four nibbles each, so it's more like this:
    //        ABCD
    //      x EFGH
    //      ------
    //   |      HD|
    //   |     HC |
    //   |    HB  |
    //   |   HA   |
    //   |     GD |
    //   |    GC  |
    //   |   GB   |
    //   |  GA    |
    //   |    FD  |
    //   |   FC   |
    //   |  FB    |
    //   | FA     |
    //   |   ED   |
    //   |  EC    |
    //   | EB     |
    //   |EA      |
    //   |        |
    //   |76543210|
    //
    // .. but again we can only output the low-order half here,
    //    so we stop computing after nibble '3',
    //    which implies we don't compute GA, FB, FA, EC, EB, EA.
    //
    //
    // I don't know how this is handling signed values.
    // Thank you, two's-complement !

    // nibbles
    u1  A = hi(a.hi);
    u1  B = lo(a.hi);
    u1  C = hi(a.lo);
    u1  D = lo(a.lo);
    u1  E = hi(b.hi);
    u1  F = lo(b.hi);
    u1  G = hi(b.lo);
    u1  H = lo(b.lo);

    u1 HD = H * D;
    u1 HC = H * C;
    u1 HB = H * B;
    u1 HA = H * A;
    u1 GD = G * D;
    u1 GC = G * C;
    u1 GB = G * B;
    u1 FD = F * D;
    u1 FC = F * C;
    u1 ED = E * D;

    // ret val 3 = high order, 0 = low order.
    u1 r3, r2, r1, r0;  // r0 = low order nibble 0
    u1 carry;
    
    // nibble 0
    r0 = 0; carry = 0;
    nibAWC(r0, lo(HD), &r0, &carry);

    // nibble 1
    r1 = carry; carry = 0;
    nibAWC(r1, hi(HD), &r1, &carry);
    nibAWC(r1, lo(HC), &r1, &carry);
    nibAWC(r1, lo(GD), &r1, &carry);

    // nibble 2
    r2 = carry; carry = 0;
    nibAWC(r2, hi(HC), &r2, &carry);
    nibAWC(r2, lo(HB), &r2, &carry);
    nibAWC(r2, hi(GD), &r2, &carry);
    nibAWC(r2, lo(GC), &r2, &carry);
    nibAWC(r2, lo(FD), &r2, &carry);
    
    // nibble 3
    r3 = carry; carry = 0;
    nibAWOC(r3, hi(HB), &r3);
    nibAWOC(r3, lo(HA), &r3);
    nibAWOC(r3, hi(GC), &r3);
    nibAWOC(r3, lo(GB), &r3);
    nibAWOC(r3, hi(FD), &r3);
    nibAWOC(r3, lo(FC), &r3);
    nibAWOC(r3, lo(ED), &r3);
    
    // this pattern can continue up to r7,
    // but those results have no home so we don't compute them.
    
    s2 ret;
    
    ret.hi = (r3 << s1_shf_hlf) | (r2);
    ret.lo = (r1 << s1_shf_hlf) | (r0);

    return ret;
}

//--------------------------------------------

const int _format_func_width = 9;

const int16_t fun_values[] = {
             0        ,
             1        ,
        -    1        ,
            51        ,
            17        ,
           255        ,
           256        ,
        -  128        ,
        -  255        ,
        -  256        ,
          1234        ,
        - 4321        ,
         12345        ,
        -12345        ,
        INT16_MIN     ,
        INT16_MIN +  1,
        INT16_MAX -  1,
        INT16_MAX     ,
    };

void test_mul(void) {
    printf("%-*s()..  ", _format_func_width, __func__);

    for (int n = 0; n < ARRAYSIZE(fun_values); ++n) {
        int16_t p = fun_values[n];
        s2 a = s2_from_int16(p);
        for (int m = 0; m < ARRAYSIZE(fun_values); ++m) {
            int16_t q     = fun_values[m];
            s2      b     = s2_from_int16(q);
            s2      c     = s2_mul(a, b);
            int16_t op_ab = int16_from_s2(c);
            int16_t op_pq = p * q;

            if (op_ab != op_pq) {
                // do it again, for breakpointing:
                s2      c     = s2_mul(a, b);
                int16_t op_ab = int16_from_s2(c);
                printf("uh oh: s2_mul(%d, %d) -> %d, expected %d\n", p, q, op_ab, op_pq);
                return;
            }
        }
    }

    printf("passed.\n");
}


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


void test_cmp(void) {
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

void test_cnv(void) {
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


void test(void) {
    test_cnv();
    test_cmp();
    test_add();
    test_sub();
    test_mul();
}

int main() {
    test();
}

