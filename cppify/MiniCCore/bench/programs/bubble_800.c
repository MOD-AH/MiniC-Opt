// Bubble sort of 800 pseudo-random ints. Loads, stores, swaps.
int v[800];
int main() {
    int i, j, seed = 12345;
    for (i = 0; i < 800; i = i + 1) { seed = (seed * 1103 + 12345) % 65536; v[i] = seed; }
    for (i = 0; i < 800; i = i + 1)
        for (j = 0; j + 1 < 800 - i; j = j + 1)
            if (v[j] > v[j + 1]) { int t = v[j]; v[j] = v[j + 1]; v[j + 1] = t; }
    return (v[0] + v[799]) % 256;
}
