/* Kadane over -2 1 -3 4 -1 2 1 -5 4; the best run is 4 -1 2 1 = 6 */
int main() {
    int a[9];
    int i; int best; int cur;
    a[0] = -2; a[1] = 1; a[2] = -3; a[3] = 4; a[4] = -1;
    a[5] = 2; a[6] = 1; a[7] = -5; a[8] = 4;
    best = a[0]; cur = a[0];
    for (i = 1; i < 9; i = i + 1) {
        if (cur + a[i] > a[i]) { cur = cur + a[i]; } else { cur = a[i]; }
        if (cur > best) { best = cur; }
    }
    return best;
}
