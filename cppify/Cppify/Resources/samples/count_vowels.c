/* "compilers!" held in a char array; the vowels are o, i, e -> 3.
   Exercises char storage plus a chain of || comparisons. */
int main() {
    char s[10];
    int i; int count;
    s[0] = 'c'; s[1] = 'o'; s[2] = 'm'; s[3] = 'p'; s[4] = 'i';
    s[5] = 'l'; s[6] = 'e'; s[7] = 'r'; s[8] = 's'; s[9] = '!';
    count = 0;
    for (i = 0; i < 10; i = i + 1) {
        if (s[i] == 'a' || s[i] == 'e' || s[i] == 'i'
                || s[i] == 'o' || s[i] == 'u') {
            count = count + 1;
        }
    }
    return count;
}
