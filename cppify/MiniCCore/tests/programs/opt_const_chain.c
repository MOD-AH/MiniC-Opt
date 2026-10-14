/* A chain where each fold feeds the next propagation:
   a=5, b=20, c=15, d=5, e=25, result 25+5 = 30 */
int main() {
    int a; int b; int c; int d; int e;
    a = 2 + 3;
    b = a * 4;
    c = b - 5;
    d = c / 3;
    e = d * d;
    return e + a;
}
