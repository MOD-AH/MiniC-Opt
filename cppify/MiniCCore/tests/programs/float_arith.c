/* 2.5 * 4.0 = 10.0 -> 10 when narrowed to int.
   7.9 truncates toward zero -> 7.  10*10 + 7 = 107 */
int main() {
    float f; float g;
    int a; int b;
    f = 2.5;
    f = f * 4.0;
    a = f;
    g = 7.9;
    b = g;
    return a * 10 + b;
}
