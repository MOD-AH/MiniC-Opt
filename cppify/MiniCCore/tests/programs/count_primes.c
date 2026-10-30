/* 15 primes below 50: 2 3 5 7 11 13 17 19 23 29 31 37 41 43 47 */
int is_prime(int n) {
    int i;
    if (n < 2) { return 0; }
    for (i = 2; i * i <= n; i = i + 1) {
        if (n % i == 0) { return 0; }
    }
    return 1;
}

int main() {
    int i; int c;
    c = 0;
    for (i = 0; i < 50; i = i + 1) { c = c + is_prime(i); }
    return c;
}
