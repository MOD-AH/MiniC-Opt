// Two P2/P3 miscompiles found by fuzzing against clang.
//   f: x = 5 is the only assignment of x, but on the other path the use
//      reads the parameter's value on entry. P2 replaced it with 5.
//   g: c = counter copies a global, then bump() changes it; P3 replaced the
//      later use of c with counter, reading the new value.
#include <stdio.h>
int counter = 3;
void bump() { counter = counter + 1; }
int f(int x, int c) {
    if (c) {
        x = 5;
    }
    return x;
}
int g() {
    int c = counter;
    bump();
    return c;
}
int main() {
    printf("%d %d %d\n", f(9, 0), f(9, 1), g());
    return 0;
}
