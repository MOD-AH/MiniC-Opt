// P7 once rebuilt the induction variable's operand as a temporary. For a
// global loop counter that read the frame instead of the global, so the
// mirror variable started at 0 and the sum came out 18 instead of 30.
int g;
int main() {
    int s = 0;
    for (g = 1; g < 5; g = g + 1) {
        s = s + g * 3;
    }
    return s;
}
