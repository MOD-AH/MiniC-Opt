#include <stdio.h>

/* Conditional compilation: only the selected branch is compiled. */
#define DEBUG
#define LEVEL 2

int main(void) {
    int x = 0;
#ifdef DEBUG
    x += 1;
#else
    x += 1000;
#endif
#ifndef DEBUG
    x += 2000;
#endif
#if 0
    this is not C at all and must never reach the parser
#endif
#if defined(MISSING)
    x += 3000;
#elif defined(LEVEL)
    x += 10;
#else
    x += 4000;
#endif
#undef DEBUG
#ifdef DEBUG
    x += 5000;
#else
    x += 100;
#endif
#if 1
#  if !defined(LEVEL)
    x += 6000;
#  else
    x += LEVEL * 1000;
#  endif
#endif
    printf("%d\n", x);       /* 1 + 10 + 100 + 2000 = 2111 */
    return 0;
}
