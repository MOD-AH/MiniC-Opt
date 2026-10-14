/* A divisor that is a literal zero is diagnosed at compile time rather
   than left to trap at run time. */
int main() {
    return 10 / 0;
}
