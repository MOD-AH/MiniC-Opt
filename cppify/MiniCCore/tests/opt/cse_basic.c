/* P4 common subexpression elimination, isolated: a and b are function
   parameters (never compile-time constants), so P1/P2/P3 cannot touch
   `a*b` at all -- only CSE can reuse the second computation. */
int combine(int a, int b) {
    int t, t2, s;
    t = a * b;
    s = t;
    t2 = a * b;
    s = s + t2;
    return s;
}
int main() {
    return combine(3, 7);
}
