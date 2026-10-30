// P8 once hoisted `a = g` out of a loop that never runs, so `a` changed
// anyway (found by fuzzing against clang). An assignment may be hoisted only
// if its variable is not live on entry to the loop: here the old `a` is read
// after the loop, and in the second loop before the assignment.
int g = 3;
int main() {
    int a = 1, b = 0, j;
    for (j = 0; j < 0; j = j + 1) {
        a = g;
    }
    for (j = 0; j < 2; j = j + 1) {
        b = b + a;
        a = g;
    }
    return a * 100 + b;
}
