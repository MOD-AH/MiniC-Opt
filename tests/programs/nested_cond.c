int classify(int x) {
    if (x < 0) {
        return -1;
    } else {
        if (x == 0) {
            return 0;
        } else {
            if (x < 100) {
                return 1;
            } else {
                return 2;
            }
        }
    }
}

int main() {
    return classify(-5) + classify(0) + classify(50) + classify(500);
}
