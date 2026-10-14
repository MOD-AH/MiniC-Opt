/* Each branch of the chain is taken exactly once:
   classify(-5)=1, classify(0)=2, classify(5)=3, classify(100)=4
   -> 1*1000 + 2*100 + 3*10 + 4 = 1234 */
int classify(int x) {
    if (x < 0) { return 1; }
    else if (x == 0) { return 2; }
    else if (x < 10) { return 3; }
    else { return 4; }
}

int main() {
    return classify(-5) * 1000 + classify(0) * 100
         + classify(5) * 10 + classify(100);
}
