/* Compound bitwise assignment and a loop-carried bit count.
   flags: 0 |= 8 -> 8 ; |= 1 -> 9 ; ^= 1 -> 8 ; &= ~8 -> 0 ; |= 15 -> 15 ; <<= 2 -> 60
   popcount(255) = 8.  1000 = 512+256+128+64+32+8, six set bits.
   return 60 & 7 = 0b111100 & 0b000111 = 4 */
int popcount(int n) {
    int c = 0;
    while (n != 0) {
        c += n & 1;
        n >>= 1;
    }
    return c;
}

int main() {
    int flags = 0;
    flags |= 1 << 3;
    flags |= 1 << 0;
    flags ^= 1 << 0;
    flags &= ~(1 << 3);
    flags |= 0x0F;
    flags <<= 2;
    printf("flags = %d\n", flags);
    printf("popcount(255) = %d\n", popcount(255));
    printf("popcount(1000) = %d\n", popcount(1000));
    return flags & 7;
}
