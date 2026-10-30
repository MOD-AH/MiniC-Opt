/* Both tests divide by a, which is zero. If && and || did not
   short-circuit, the right operand would evaluate and the interpreter
   would trap on division by zero instead of returning 12. That makes
   this a behavioural test, not just an arithmetic one. */
int main() {
    int a; int b; int r;
    a = 0; b = 5; r = 0;
    if (a != 0 && 10 / a > 1) { r = 1; } else { r = 2; }
    if (b > 0 || 10 / a > 1) { r = r + 10; }
    return r;
}
