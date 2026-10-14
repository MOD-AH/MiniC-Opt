/* Bottom-up Fibonacci in an array, f[0]=0, so f[29] = 514229.
   The recursive fib_rec.c computes the same series the slow way. */
int main() {
    int f[30];
    int i;
    f[0] = 0; f[1] = 1;
    for (i = 2; i < 30; i = i + 1) { f[i] = f[i - 1] + f[i - 2]; }
    return f[29];
}
