/* LICM and strength reduction in one loop: a*b is invariant (42) and
   i*4 is an induction-variable multiply. sum over i=0..9 of (42 + 4i)
   = 10*42 + 4*45 = 600 */
int main() {
    int i; int s; int a; int b; int inv;
    a = 6; b = 7; s = 0;
    for (i = 0; i < 10; i = i + 1) {
        inv = a * b;
        s = s + inv + i * 4;
    }
    return s;
}
