// Out-of-SSA correctness. After GVN folds the copies, the swap loop leaves
// two phis that read each other (the swap problem), and the second loop
// needs a value from before the update after the loop (the lost-copy
// problem). Naive phi removal gets both wrong; the copies plus coalescing
// in src/ssa/ssa.cpp must not.
#include <stdio.h>
int main() {
    int a = 1, b = 2, i, t;
    for (i = 0; i < 5; i = i + 1) {
        t = a;
        a = b;
        b = t;
    }
    int x = 0, y = 0;
    for (i = 0; i < 4; i = i + 1) {
        y = x;
        x = x + 1;
    }
    printf("%d %d %d %d\n", a, b, x, y);
    return a * 10 + b;
}
