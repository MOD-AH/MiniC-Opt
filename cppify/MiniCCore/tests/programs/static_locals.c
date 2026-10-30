#include <stdio.h>

/* static locals keep their value between calls and are initialized once. */
int counter(void) {
    static int n = 10;
    n++;
    return n;
}

int sum_calls(int x) {
    static int total;            /* zero-initialized */
    static int history[4];
    history[total % 4] = x;
    total += x;
    return total;
}

int twin(int which) {
    /* two statics with the same name in different blocks are different objects */
    if (which) { static int k = 100; return ++k; }
    else       { static int k = 0;   return ++k; }
}

int *stable_address(void) {
    static int cell = 7;
    return &cell;                /* address-taken static: same cell every call */
}

int main(void) {
    counter(); counter();
    printf("%d\n", counter());                       /* 13 */
    sum_calls(5); sum_calls(6);
    printf("%d\n", sum_calls(7));                    /* 18 */
    printf("%d %d %d %d\n", twin(1), twin(1), twin(0), twin(0));   /* 101 102 1 2 */
    int *p = stable_address();
    *p = 42;
    printf("%d %d\n", *stable_address(), p == stable_address());   /* 42 1 */
    return counter();                                /* 14 */
}
