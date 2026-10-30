// 48x48 integer matrix product, twice. Nested loops, 2-D indexing.
int a[48][48], b[48][48], c[48][48];
int main() {
    int i, j, k, r, s = 0;
    for (i = 0; i < 48; i = i + 1)
        for (j = 0; j < 48; j = j + 1) { a[i][j] = i + j; b[i][j] = i - j; }
    for (r = 0; r < 2; r = r + 1)
        for (i = 0; i < 48; i = i + 1)
            for (j = 0; j < 48; j = j + 1) {
                int t = 0;
                for (k = 0; k < 48; k = k + 1) t = t + a[i][k] * b[k][j];
                c[i][j] = t;
            }
    for (i = 0; i < 48; i = i + 1) s = s ^ c[i][i];
    return s & 255;
}
