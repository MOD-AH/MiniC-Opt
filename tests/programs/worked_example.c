/* The report's own worked optimization example (docs/worked_example.md),
   promoted to a committed benchmark now that the full P1-P9 pipeline
   exists to run it through. P2 propagates a=4,b=5,n=100; P1 folds the
   resulting 4*5 and 1*8; whatever's left of the loop-invariant multiply
   is either resolved by propagation or hoisted by P8; P4 reuses any
   remaining redundant recomputation; P5 deletes the now-dead a, b, n.
   s accumulates 40 per iteration for 13 iterations (i = 0, 8, ..., 96). */
int main() {
    int i, n, s, t, a, b;
    a = 4;  b = 5;  n = 100;  s = 0;
    i = 0;
    while (i < n) {
        t = a * b;
        s = s + t;
        s = s + a * b;
        i = i + 1 * 8;
    }
    return s;
}
