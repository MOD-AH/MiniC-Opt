// Longest Collatz chain below 30000. Scalar arithmetic and branches only.
int main() {
    int n, best = 0, arg = 0;
    for (n = 1; n < 30000; n = n + 1) {
        long x = n;
        int len = 1;
        while (x != 1) {
            if (x % 2 == 0) x = x / 2; else x = 3 * x + 1;
            len = len + 1;
        }
        if (len > best) { best = len; arg = n; }
    }
    return (arg + best) % 256;
}
