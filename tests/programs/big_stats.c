/* sum = 118, 118/8 = 14 (integer division), max 42, min 3
   -> 14*1000 + 42*10 + 3 = 14423 */
int main() {
    int a[8];
    int i; int mn; int mx; int sum;
    a[0] = 4; a[1] = 8; a[2] = 15; a[3] = 16;
    a[4] = 23; a[5] = 42; a[6] = 7; a[7] = 3;
    mn = a[0]; mx = a[0]; sum = 0;
    for (i = 0; i < 8; i = i + 1) {
        if (a[i] < mn) { mn = a[i]; }
        if (a[i] > mx) { mx = a[i]; }
        sum = sum + a[i];
    }
    return (sum / 8) * 1000 + mx * 10 + mn;
}
