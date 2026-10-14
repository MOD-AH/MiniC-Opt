/* A real (non-micro-test) benchmark exercising nested loops with an
   inner break -- the same shape that produces a genuine jump-to-a-jump
   chain for P9 peephole to collapse (see tests/opt/peephole_basic.c's
   header comment for exactly why). */
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
