// Loads that look invariant but must stay in the loop.
//   1. a[1] is stored to in the same loop.
//   2. a[0] is stored to through p, a pointer derived from a.
//   3. a[idx] is out of bounds; the loop never runs, so it must never load.
//   4. g[0] may be changed by bump(), a call in the loop.
#include <stdio.h>
int g[2];
void bump() { g[0] = g[0] + 1; }
int main() {
    int a[3];
    int i, s = 0, n = 0, idx = 7;
    int *p = a;
    a[0] = 1; a[1] = 2; a[2] = 3;
    for (i = 0; i < 5; i = i + 1) {
        s = s + a[1];
        a[1] = a[1] + i;
    }
    for (i = 0; i < 3; i = i + 1) {
        s = s + a[0];
        p[0] = p[0] * 2;
    }
    while (n > 0 && a[idx] > 0) {
        n = n - 1;
    }
    for (i = 0; i < 3; i = i + 1) {
        s = s + g[0];
        bump();
    }
    printf("%d %d %d\n", s, a[0], a[1]);
    return 0;
}
