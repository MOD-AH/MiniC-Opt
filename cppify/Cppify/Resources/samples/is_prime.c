/* 97 is prime, 91 = 7*13 is not, 2 is prime
   -> 1*100 + 0*10 + 1 = 101 */
int is_prime(int n) {
    int i;
    if (n < 2) { return 0; }
    for (i = 2; i * i <= n; i = i + 1) {
        if (n % i == 0) { return 0; }
    }
    return 1;
}

int main() {
    return is_prime(97) * 100 + is_prime(91) * 10 + is_prime(2);
}
