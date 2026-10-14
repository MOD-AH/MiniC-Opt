int global_val = 5;

int optimize_me(int n) {
    int x = 10;
    int y = x;
    int a = 60 * 60;
    int b = 0;

    if (b != 0) {
        global_val = 999;
    }

    int base = n * 8;
    int result = 0;

    for (int i = 0; i < 10; i++) {
        int invariant = x + global_val;
        int step1 = i * (y + 2);
        int step2 = i * (y + 2) + base;
        result += step1 + step2 + invariant;
    }

    return result;
}

int main() {
    print_int(optimize_me(5));
    return 0;
}
