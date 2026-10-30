// `for (int i = 0; ; i++)`: a declaration in the init clause and an empty
// condition. The parser used to skip its declaration path when the condition
// was empty, then read `i++` as the condition and reject the program.
#include <stdio.h>
int main() {
    int s = 0;
    for (int i = 0; ; i++) {
        if (i == 5) break;
        s = s + i;
    }
    for (int j = 3, k = 0; j > 0; ) {
        j--;
        k++;
        s = s + k;
    }
    printf("%d\n", s);
    return s;
}
