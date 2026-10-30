/* Recovery: a missing ';' right before '}' must not consume the brace.
   Expect exactly 1 error. */
int twice(int x) {
    int y = x * 2
}

int main(void) { return twice(3); }
