/* Recursion depth 1000 -- the activation-record stack must not blow up. */
int depth(int n) {
    if (n <= 0) { return 0; }
    return 1 + depth(n - 1);
}

int main() {
    return depth(1000);
}
