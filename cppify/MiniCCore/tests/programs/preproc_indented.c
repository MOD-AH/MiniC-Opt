// An indented directive inside a skipped #if region still counts. Before the
// fix the skipped region dropped every line not starting with '#', so the
// `  #endif` below was lost and the whole file reported an unterminated #if.
#include <stdio.h>
#if 0
int broken = ;
  #endif
    #ifdef NULL
int seen = 7;
    #else
int seen = 0;
    #endif
int main() {
    printf("%d\n", seen);
    return seen;
}
