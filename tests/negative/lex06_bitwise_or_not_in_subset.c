/* Bitwise operators are outside the MiniC subset; '&' is covered by
   lex04, this is the '|' half. */
int main() {
    int a;
    a = 1 | 2;
    return a;
}
