// goto in every shape the AST interpreter has to handle by re-entering
// statements (roadmap 3.1): backwards, forwards out of nested loops, into the
// middle of a loop body, and inside a switch. Checked against clang.
#include <stdio.h>
int main() {
    int i = 0, s = 0, j;
again:
    s = s + i;
    i = i + 1;
    if (i < 5) goto again;               // backwards: 0+1+2+3+4 = 10

    for (i = 0; i < 4; i = i + 1)
        for (j = 0; j < 4; j = j + 1)
            if (i * j == 6) goto out;    // forwards, out of two loops
out:
    printf("%d %d %d\n", s, i, j);       // 10 2 3

    int k = 0;
    goto inside;                         // into the middle of a loop body
    while (k < 3) {
        k = k + 10;
inside:
        k = k + 1;
    }
    printf("%d\n", k);                   // 1 on entry, then one full pass: 12

    int n = 0, v = 2;
    switch (v) {
        case 1: n = n + 100;
        case 2: n = n + 1;
            if (n < 3) goto retry;
            break;
        case 3:
retry:
            n = n + 1;
            goto done;
    }
done:
    printf("%d\n", n);                   // case 2 gives 1, retry adds 1: 2
    return s + k + n;
}
