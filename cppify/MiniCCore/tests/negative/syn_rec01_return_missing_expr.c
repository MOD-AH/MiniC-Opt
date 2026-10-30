/* Recovery: one fault, one diagnostic. The parser used to swallow the '}'
   while recovering, leave factorial's body open, and report two more
   bogus errors about main. Expect exactly 1 error. */
unsigned long long factorial(int n) {
    if (n <= 1) return 1;
    return
}

int main(void) { return (int)factorial(5); }
