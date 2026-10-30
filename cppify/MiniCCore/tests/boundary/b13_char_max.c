/* 127 is the largest value that is unambiguous whether char is signed
   or unsigned, so this pins the boundary without depending on which. */
int main() {
    char c;
    c = 127;
    return c;
}
