/* 100 nested if statements; statement nesting is far cheaper on the
   stack than expression nesting, but must still not crash. */
int main() {
    int x;
    x = 0;
    if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) {if (1) { x = 1; }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
    return x;
}
