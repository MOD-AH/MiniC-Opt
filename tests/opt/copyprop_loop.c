/* P3 micro-test 3 — copying a LOOP variable (changes every iteration, so
   P2's constant test never applies at all): x = i; y = x; sum = sum + y,
   for i = 0,1,2 -> sum = 0+1+2 = 3. Isolates P3 from P2 entirely. */
int main() {
    int i;
    int x;
    int y;
    int sum;
    sum = 0;
    for (i = 0; i < 3; i = i + 1) {
        x = i;
        y = x;
        sum = sum + y;
    }
    return sum;
}
