int main() {
    int a[9];
    int b[9];
    int c[9];
    int i;
    int j;
    int k;
    int n = 3;

    for (i = 0; i < 9; i = i + 1) {
        a[i] = i + 1;
        b[i] = i + 1;
        c[i] = 0;
    }

    for (i = 0; i < n; i = i + 1) {
        for (j = 0; j < n; j = j + 1) {
            int sum = 0;
            for (k = 0; k < n; k = k + 1) {
                sum = sum + a[i * n + k] * b[k * n + j];
            }
            c[i * n + j] = sum;
        }
    }
    return c[8];
}
