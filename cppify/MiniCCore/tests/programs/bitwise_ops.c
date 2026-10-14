/* & | ^ ~ << >> and hex literals.

   The p1..p5 lines are chosen so the CORRECT C grouping and the most likely
   WRONG grouping give different answers, otherwise they would not test
   precedence at all:
     p1  4 | 6 & 3    = 4 | (6&3)  = 6        wrong (4|6)&3  = 2
     p2  6 ^ 3 & 1    = 6 ^ (3&1)  = 7        wrong (6^3)&1  = 1
     p3  1 << 2 + 1   = 1 << (2+1) = 8        wrong (1<<2)+1 = 5
     p4  5 & 3 == 3   = 5 & (3==3) = 1        wrong (5&3)==3 = 0
     p5  1 | 2 ^ 3    = 1 | (2^3)  = 1        wrong (1|2)^3  = 0 */
int main() {
    int a = 0xFF;
    int b = 12;
    printf("and  %d\n", a & b);
    printf("or   %d\n", a | 256);
    printf("xor  %d\n", b ^ 5);
    printf("not  %d\n", ~b);
    printf("shl  %d\n", b << 3);
    printf("shr  %d\n", a >> 2);
    printf("p1   %d\n", 4 | 6 & 3);
    printf("p2   %d\n", 6 ^ 3 & 1);
    printf("p3   %d\n", 1 << 2 + 1);
    printf("p4   %d\n", 5 & 3 == 3);
    printf("p5   %d\n", 1 | 2 ^ 3);
    printf("hex  %d\n", 0x10 + 0xA);
    if ((a & 1) == 1) { printf("low bit set\n"); }
    return 0;
}
