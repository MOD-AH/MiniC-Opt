// Counts primes below 200000 with a sieve. Array-heavy inner loop.
int flags[200000];
int main() {
    int i, j, count = 0;
    for (i = 2; i < 200000; i = i + 1) flags[i] = 1;
    for (i = 2; i * i < 200000; i = i + 1)
        if (flags[i])
            for (j = i * i; j < 200000; j = j + i) flags[j] = 0;
    for (i = 2; i < 200000; i = i + 1) count = count + flags[i];
    return count % 256;
}
