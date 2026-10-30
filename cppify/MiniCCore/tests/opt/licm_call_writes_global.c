// P8 once hoisted `t = d ^ g` out of this loop: g is never assigned in the
// loop's own code, but bump() writes it on every iteration. Found by
// fuzzing against clang.
int g = 0;
void bump() { g = g + 1; }
int main() {
    int d = 5, e = 0, i;
    for (i = 0; i < 6; i = i + 1) {
        bump();
        e = e + (d ^ g);
    }
    return e;
}
