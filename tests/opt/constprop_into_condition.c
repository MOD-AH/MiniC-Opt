/* P2 micro-test 3 — a constant propagated into a branch CONDITION, not
   just an arithmetic expression (n == 0 folds to true once n's constant
   is propagated in). */
int main() {
    int n;
    int x;
    n = 0;
    if (n == 0) {
        x = 1;
    } else {
        x = 2;
    }
    return x;
}
