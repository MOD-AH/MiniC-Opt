/* Structs, typedef, enum, #define, 2-D arrays, ?:, comma, switch fall-through,
   sizeof and goto, all in one program. Hand-computed:
     add -> (11,22); bumpx has no effect (by value); movex +5 -> (16,22)
     sum of x*y over {1,1},{2,4},{3,9} = 1+8+27 = 36
     d is a copy of pts[2]: pts[2].x stays 3, d.x = 100
     list 1->2->3 sums to 6
     m rows: (1+2+3)*1 + (4+5+6)*2 = 36, m[1][2] = 6
     enum: RED=0 GREEN=5 BLUE=6
     switch(ON): prints "on" then falls into default "fall"
     k = (i=5, i*2) = 10
     sizeof(Point)=8 sizeof(int)=4, SQ + N = 3*3 + 4 = 13
     goto loop runs cnt up to 3; return a.x + b.y = 1 + 20 = 21 */
#include <stdio.h>
#define N 4
#define SQ 3 * 3
typedef struct { int x; int y; } Point;
struct Node { int val; struct Node *next; };
enum Color { RED, GREEN = 5, BLUE };
typedef enum { OFF, ON } Switch;

Point add(Point a, Point b) { Point r; r.x = a.x + b.x; r.y = a.y + b.y; return r; }
void bumpx(Point p) { p.x = 999; }
void movex(Point *p, int dx) { p->x += dx; }
int sumlist(struct Node *h) { int s = 0; while (h != NULL) { s += h->val; h = h->next; } return s; }

int main(void) {
    Point a = {1, 2};
    Point b = {10, 20};
    Point c = add(a, b);
    printf("%d %d\n", c.x, c.y);
    bumpx(c);
    movex(&c, 5);
    printf("%d %d\n", c.x, c.y);
    Point pts[3] = {{1, 1}, {2, 4}, {3, 9}};
    int i, t = 0;
    for (i = 0; i < 3; i++) t += pts[i].x * pts[i].y;
    printf("%d\n", t);
    Point d = pts[2];
    d.x = 100;
    printf("%d %d\n", pts[2].x, d.x);
    struct Node n3 = {3, NULL};
    struct Node n2 = {2, &n3};
    struct Node n1 = {1, &n2};
    printf("%d\n", sumlist(&n1));
    int m[2][3] = {{1, 2, 3}, {4, 5, 6}};
    int s = 0, j;
    for (i = 0; i < 2; i++) for (j = 0; j < 3; j++) s += m[i][j] * (i + 1);
    printf("%d %d\n", s, m[1][2]);
    printf("%d %d %d\n", RED, GREEN, BLUE);
    Switch sw = ON;
    switch (sw) { case OFF: printf("off\n"); break; case ON: printf("on\n"); default: printf("fall\n"); }
    int k = (i = 5, i * 2);
    printf("%d %d\n", k, k > 5 ? 1 : 0);
    printf("%d %d %d\n", (int)sizeof(Point), (int)sizeof(int), SQ + N);
    int cnt = 0;
loop:
    cnt++;
    if (cnt < 3) goto loop;
    printf("%d\n", cnt);
    return a.x + b.y;
}
