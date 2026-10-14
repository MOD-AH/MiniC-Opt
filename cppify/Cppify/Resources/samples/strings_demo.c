/* String handling with pointers and <string.h>: reverse, count words,
   palindrome check, and building a string with sprintf. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

void reverse(char *s) {
    char *e = s + strlen(s) - 1;
    while (s < e) { char t = *s; *s++ = *e; *e-- = t; }
}

int words(const char *s) {
    int n = 0, in = 0;
    for (; *s; s++) {
        if (isspace(*s)) in = 0;
        else if (!in) { in = 1; n++; }
    }
    return n;
}

int is_palindrome(const char *s) {
    int i = 0, j = strlen(s) - 1;
    while (i < j) if (tolower(s[i++]) != tolower(s[j--])) return 0;
    return 1;
}

int main() {
    char word[32] = "compiler";
    reverse(word);
    printf("%s\n", word);
    printf("%d words\n", words("  the quick  brown fox "));
    printf("%d %d\n", is_palindrome("Racecar"), is_palindrome("Cppify"));
    char buf[64];
    sprintf(buf, "%s has %d letters", "optimizer", (int)strlen("optimizer"));
    puts(buf);
    return 0;
}
