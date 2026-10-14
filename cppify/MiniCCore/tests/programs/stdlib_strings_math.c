#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
struct Node { int val; struct Node *next; };
int main() {
    char name[20] = "world";
    char greet[40];
    strcpy(greet, "hello ");
    strcat(greet, name);
    printf("%s|%d|%5d|%-5d|%05d\n", greet, (int)strlen(greet), 42, 42, 42);
    printf("%.2f %8.3f %e %g\n", 3.14159, 2.5, 12345.678, 0.0001);
    printf("%x %X %o %c %5s %-6s| %%\n", 255, 255, 8, 'A', "ab", "cd");
    char *p = strchr(greet, 'w');
    printf("%s %d\n", p, strcmp("abc", "abd"));
    int *a = (int *)malloc(5 * sizeof(int));
    int i;
    for (i = 0; i < 5; i++) a[i] = i * i;
    int s = 0;
    for (i = 0; i < 5; i++) s += a[i];
    printf("%d\n", s);
    free(a);
    struct Node *n1 = (struct Node *)malloc(sizeof(struct Node));
    struct Node *n2 = (struct Node *)malloc(sizeof(struct Node));
    n1->val = 7; n1->next = n2; n2->val = 8; n2->next = NULL;
    printf("%d %d\n", n1->val, n1->next->val);
    printf("%.3f %.1f %d %d\n", sqrt(2.0), pow(2, 10), abs(-7), (int)floor(3.9));
    char up[10]; strcpy(up, "MixEd");
    for (i = 0; up[i]; i++) up[i] = toupper(up[i]);
    puts(up);
    int arr[4] = {1, 2, 3, 4};
    memset(arr, 0, sizeof(arr));
    printf("%d %d\n", arr[0], arr[3]);
    char buf[50];
    sprintf(buf, "%d-%s-%.1f", 5, "x", 2.25);
    printf("%s\n", buf);
    int x, y; char w[20];
    int got = sscanf("12 34 word", "%d %d %s", &x, &y, w);
    printf("%d %d %d %s\n", got, x, y, w);
    printf("%d\n", atoi("123") + 1);
    return strlen("abc");
}
