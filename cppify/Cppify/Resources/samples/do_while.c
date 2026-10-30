/* do-while: the body must run at least once even when the condition is
   false from the start. i reaches 5 by the condition; j proves the
   run-at-least-once rule with a condition that is false immediately.
   5*10 + 1 = 51 */
int main() {
    int i; int j;
    i = 0;
    do { i = i + 1; } while (i < 5);
    j = 0;
    do { j = j + 1; } while (0);
    return i * 10 + j;
}
