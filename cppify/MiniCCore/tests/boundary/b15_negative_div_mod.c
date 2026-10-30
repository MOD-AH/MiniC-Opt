/* C truncates toward zero: -7/2 == -3 and -7%3 == -1.
   Combined so a wrong sign on either shows up in the result. */
int main() {
    int a; int b; int c;
    a = -7; b = 2; c = 3;
    return (a / b) * 10 + (a % c);
}
