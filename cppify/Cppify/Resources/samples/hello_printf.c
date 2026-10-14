#include <stdio.h>

int add(int a, int b) { return a + b; }

int main() {
    int x = 7;
    float f = 2.5;
    printf("Hello from MiniC!\n");
    printf("x = %d, add(x,3) = %d\n", x, add(x, 3));
    printf("float %f and char %c and literal %%\n", f, 65);
    return 0;
}
