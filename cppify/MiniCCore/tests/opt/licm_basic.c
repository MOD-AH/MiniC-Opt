/* P8 loop-invariant code motion, isolated: a and b are function
   parameters (never compile-time constants, so P1/P2 can't fold a*b
   into a literal the way the worked example's constants let them) --
   the multiply is genuinely invariant across the loop and only LICM has
   a reason to hoist it. */
int loopy(int a, int b, int n) {
    int i, s;
    i = 0;
    s = 0;
    while (i < n) {
        s = s + a * b;
        i = i + 1;
    }
    return s;
}
int main() {
    return loopy(3, 4, 5);
}
