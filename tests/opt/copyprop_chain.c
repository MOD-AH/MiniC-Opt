/* P3 micro-test 2 — a copy CHAIN on top of one constant: a = 7; b = a;
   c = b; d = c; return d. Needs several sweeps of P2 (for a) and P3 (for
   the b/c/d hops) cooperating to fully collapse to 7. */
int main() {
    int a;
    int b;
    int c;
    int d;
    a = 7;
    b = a;
    c = b;
    d = c;
    return d;
}
