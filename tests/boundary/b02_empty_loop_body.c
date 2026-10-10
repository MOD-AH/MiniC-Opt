/* Loop body is an empty compound statement; the work happens in the
   for-header. Exercises a basic block with no instructions in it. */
int main() {
    int i;
    for (i = 0; i < 5; i = i + 1) { }
    return i;
}
