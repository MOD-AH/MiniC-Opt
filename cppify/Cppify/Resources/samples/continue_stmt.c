/* continue skips the accumulate for even i, leaving the odd numbers
   1+3+5+7+9 = 25 */
int main() {
    int s; int i;
    s = 0; i = 0;
    while (i < 10) {
        i = i + 1;
        if (i % 2 == 0) { continue; }
        s = s + i;
    }
    return s;
}
