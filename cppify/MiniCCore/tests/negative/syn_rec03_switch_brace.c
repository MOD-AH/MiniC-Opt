/* Recovery inside a switch body: same rule as a block. Expect exactly 1 error. */
int main(void) {
    int x = 0;
    switch (x) {
        case 0: x = }
    return x;
}

int other(void) { return 1; }
