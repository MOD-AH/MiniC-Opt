/* p*q is invariant with respect to BOTH loops, so hoisting it once is
   only correct if the preheader analysis handles the nesting.
   4 * 5 * 12 = 240 */
int main() {
    int i; int j; int s; int p; int q;
    p = 3; q = 4; s = 0;
    for (i = 0; i < 4; i = i + 1) {
        for (j = 0; j < 5; j = j + 1) {
            s = s + p * q;
        }
    }
    return s;
}
