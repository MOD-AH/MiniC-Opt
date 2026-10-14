#include <stdio.h>
#include <string.h>
int main() {
    int n, i, total = 0;
    char name[32];
    float avg;
    if (scanf("%d", &n) != 1) return 1;
    for (i = 0; i < n; i++) { int v; scanf("%d", &v); total += v; }
    scanf("%s", name);
    getchar();
    char line[64];
    fgets(line, sizeof(line), stdin);
    line[strcspn(line, "\n")] = '\0';
    avg = (float)total / n;
    printf("%s: n=%d total=%d avg=%.2f\n", name, n, total, avg);
    printf("[%s] len=%d\n", line, (int)strlen(line));
    char c;
    int r = scanf(" %c", &c);
    printf("%d %c\n", r, c);
    r = scanf("%d", &n);
    printf("eof=%d\n", r);
    return 0;
}
