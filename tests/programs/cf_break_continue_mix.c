/* continue skips i==3 entirely; the inner loop always breaks at j==2.
   4 surviving outer iterations * 2 inner increments = 8 */
int main() {
    int i; int j; int s;
    s = 0;
    for (i = 0; i < 5; i = i + 1) {
        if (i == 3) { continue; }
        for (j = 0; j < 5; j = j + 1) {
            if (j == 2) { break; }
            s = s + 1;
        }
    }
    return s;
}
