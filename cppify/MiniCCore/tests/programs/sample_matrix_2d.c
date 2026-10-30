/* Two-dimensional arrays: initialise, multiply, transpose, print. */
#include <stdio.h>
#define N 3

void multiply(int a[][N], int b[][N], int c[][N]) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            c[i][j] = 0;
            for (int k = 0; k < N; k++) c[i][j] += a[i][k] * b[k][j];
        }
}

int main() {
    int a[N][N] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int id[N][N] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    int c[N][N];
    multiply(a, a, c);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) printf("%5d", c[i][j]);
        printf("\n");
    }
    multiply(a, id, c);
    int same = 1;
    for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) if (c[i][j] != a[i][j]) same = 0;
    printf("A x I == A ? %s\n", same ? "yes" : "no");
    return 0;
}
