// Roadmap 5.5: three places Cppify used to differ from clang on valid C.
//   sizeof(double) was 4; it is 8, and a struct with a double is laid out
//   with 8-byte alignment.
//   sizeof had type int; it is size_t (unsigned long), so -1 < sizeof(int)
//   compares as unsigned and is 0.
//   Bytes above 127 in a string were stored 128-255; char is signed, so they
//   are negative, as clang gives.
#include <stdio.h>
#include <string.h>
struct Mixed { char tag; double value; int count; };
int main() {
    double d = 2.5;
    float f = 1.5;
    printf("%d %d %d %d\n", (int)sizeof(double), (int)sizeof(d), (int)sizeof(f), (int)sizeof(3.14));
    printf("%d %d\n", (int)sizeof(struct Mixed), (int)sizeof(d * f));
    printf("%d %d\n", -1 < sizeof(int), (int)(sizeof(int) - 5 > 0));
    char s[] = "hé!";      /* é is two UTF-8 bytes, 0xC3 0xA9 */
    char t[8];
    strcpy(t, "€");        /* 0xE2 0x82 0xAC */
    printf("%d %d %d %d\n", s[1], s[2], t[0], (int)strlen(s));
    printf("%d\n", s[1] < 0);
    return (int)sizeof(struct Mixed);
}
