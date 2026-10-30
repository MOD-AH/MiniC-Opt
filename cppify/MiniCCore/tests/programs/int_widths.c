#include <stdio.h>

/* Integer widths and signedness, checked against clang. Every case here is
   defined C (unsigned wraparound, conversions) or implementation-defined
   with one universal answer on two's-complement targets. */
unsigned char bump(unsigned char c) { return c + 1; }
short widen(short s) { return s; }

int main(void) {
    unsigned char uc = 250; uc += 10;              /* 4 */
    signed char sc = 127; sc++;                    /* -128 */
    char c = (char)200;                                 /* -56: char is signed here */
    short s = 32767; s = s + 1;                    /* -32768 */
    unsigned short us = 0; us--;                   /* 65535 */
    int i = 2147483647; unsigned u = i; u = u + 1; /* 2147483648 */
    unsigned int z = 0; z = z - 1;                 /* 4294967295 */
    long l = 2147483647; l = l + 1;                /* 2147483648 */
    long long big = 9223372036854775807LL;
    unsigned long long ull = 0; ull = ull - 1;     /* 18446744073709551615 */
    printf("%d %d %d %d %d %u %u %ld %lld %llu\n", uc, sc, c, s, us, u, z, l, big, ull);

    /* usual arithmetic conversions: -1 becomes UINT_MAX against unsigned */
    printf("%d %d %d\n", -1 < 0u, -1 < 0, (unsigned)-1 > 1);
    printf("%d %d\n", -1L < 0u, ull > 1);          /* long holds every unsigned value: 1; u64 max > 1 */
    printf("%u %llu\n", z / 2u, ull / 3);          /* unsigned divide */
    printf("%u %llu\n", z >> 28, ull >> 60);       /* logical shift */
    printf("%d %d\n", -16 >> 2, -7 / 2);           /* arithmetic shift, truncating divide */

    /* integer promotion: char arithmetic happens in int */
    unsigned char a = 200, b = 100;
    int sum = a + b;                               /* 300 */
    unsigned char wrapped = a + b;                 /* 44 */
    printf("%d %d %d\n", sum, wrapped, bump(255));

    /* explicit casts narrow */
    printf("%d %d %d %u\n", (char)300, (unsigned char)-1, (short)70000, (unsigned)-2);
    printf("%d %d\n", widen((short)40000), (int)3000000000u);

    /* compound assignment computes in the wider type, then narrows */
    int x = -7; unsigned two = 2; x /= two;         /* (unsigned)-7 / 2 = 2147483644 */
    char ch = 100; ch *= 3;                         /* 300 -> 44 */
    unsigned char sh = 1; sh <<= 9;                 /* 512 -> 0 */
    printf("%d %d %d\n", x, ch, sh);

    /* hash-style multiply that overflows int */
    unsigned int h = 2166136261u;
    for (int k = 0; k < 5; k++) h = (h ^ (unsigned)k) * 16777619u;
    int hs = 1;
    for (int k = 0; k < 40; k++) hs = hs * 31 + k;   /* wraps mod 2^32 */
    printf("%u %d\n", h, hs);

    /* sizeof follows the widths */
    printf("%d %d %d %d %d\n", (int)sizeof(char), (int)sizeof(short), (int)sizeof(int),
           (int)sizeof(long), (int)sizeof(long long));

    /* literals take their C type */
    printf("%ld %d\n", 3000000000 * 2, 2147483647 + 0 == 0x7fffffff);
    return 0;
}
