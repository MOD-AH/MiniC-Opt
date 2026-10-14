/* Pointers: swap through &x, array walking with ++ and +=, pointer
   difference and comparison, pointer-to-pointer, NULL. Hand-computed:
   swap -> 9 3; p=a+1,*p=20,p+=2 -> *p=4, a[1]=20, sum=33;
   q-p=1, p<q=1; **pp=77 -> a[3]=77. */
void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; }
int sum(int *p, int n) { int s = 0; int i; for (i = 0; i < n; i++) s += p[i]; return s; }
int main() {
    int x = 3, y = 9;
    swap(&x, &y);
    printf("%d %d\n", x, y);
    int a[5] = {1, 2, 3, 4, 5};
    int *p = a;
    p++;
    *p = 20;
    p += 2;
    printf("%d %d %d\n", *p, a[1], sum(a, 5));
    int *q = &a[4];
    printf("%d %d\n", q - p, p < q);
    int **pp = &p;
    **pp = 77;
    printf("%d\n", a[3]);
    int *n = 0;
    if (!n) printf("null\n");
    return 0;
}
