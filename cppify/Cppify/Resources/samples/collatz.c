/* The Collatz sequence starting at 27 takes 111 steps to reach 1. */
int main() {
    int n; int steps;
    n = 27; steps = 0;
    while (n != 1) {
        if (n % 2 == 0) { n = n / 2; } else { n = 3 * n + 1; }
        steps = steps + 1;
    }
    return steps;
}
