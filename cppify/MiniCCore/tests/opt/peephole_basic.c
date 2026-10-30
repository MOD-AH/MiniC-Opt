/* P9 peephole, isolated: an inner loop's `break` jumps to the inner
   loop's own exit label, which -- because the inner while is the very
   last statement of the outer loop's body -- contains nothing but an
   unconditional goto back to the outer loop's header. Only peephole has
   a reason to collapse that jump-to-a-jump. */
int main() {
    int i, j, s;
    s = 0;
    i = 0;
    while (i < 3) {
        i = i + 1;
        j = 0;
        while (j < 3) {
            if (j == 1) {
                break;
            }
            s = s + 1;
            j = j + 1;
        }
    }
    return s;
}
