/* a: 5 then a++ -> 6.  b: copy of a then ++b -> 7.  c: copy of b then
   c-- -> 6.  600 + 70 + 6 = 676 */
int main() {
    int a; int b; int c;
    a = 5;
    a++;
    b = a;
    ++b;
    c = b;
    c--;
    return a * 100 + b * 10 + c;
}
