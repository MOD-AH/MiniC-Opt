/* P1 micro-test 3 — MOD and a unary NEG of a constant, both foldable:
   17 % 5 -> 2; -(-7) -> 7 (needs two sweeps: inner NEG folds first). */
int main() {
    int x;
    int y;
    x = 17 % 5;
    y = -(-7);
    return x + y;
}
