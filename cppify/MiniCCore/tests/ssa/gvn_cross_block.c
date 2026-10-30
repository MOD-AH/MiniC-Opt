// a * b is computed in the entry block and again in both arms of the if.
// The arms are dominated by the entry block, so GVN reuses the first product;
// P4 only looks within one block and finds nothing.
#include <stdio.h>
int f(int a, int b, int c) {
    int x = a * b + c;
    int y;
    if (c > 0) {
        y = a * b + 1;
    } else {
        y = a * b - 1;
    }
    return x + y;
}
int main() {
    printf("%d %d\n", f(3, 4, 5), f(6, 7, -1));
    return 0;
}
