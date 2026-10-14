/* P3 micro-test 1 — copying a PARAMETER (a genuinely non-constant value,
   so P2 cannot touch this at all): b = a; return b + 1 -> return a + 1. */
int identity(int a) {
    int b;
    b = a;
    return b + 1;
}
int main() {
    return identity(5);
}
