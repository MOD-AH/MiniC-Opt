/* P5 dead-code elimination, isolated: x is computed from parameters
   (so P1/P2/P3/P4 have nothing foldable/reusable to do with it) and
   then never read again -- only DCE has any reason to remove it. */
int f(int a, int b) {
    int x, y;
    x = a * b;
    y = a + b;
    return y;
}
int main() {
    return f(3, 4);
}
