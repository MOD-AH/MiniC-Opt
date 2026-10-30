/* Sieve of Eratosthenes below 100. There are 25 such primes. */
int main() {
    int mark[100];
    int i; int j; int count;
    for (i = 0; i < 100; i = i + 1) { mark[i] = 1; }
    mark[0] = 0; mark[1] = 0;
    for (i = 2; i * i < 100; i = i + 1) {
        if (mark[i] == 1) {
            for (j = i * i; j < 100; j = j + i) { mark[j] = 0; }
        }
    }
    count = 0;
    for (i = 0; i < 100; i = i + 1) { count = count + mark[i]; }
    return count;
}
