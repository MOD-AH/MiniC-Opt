int main() {
    int mark[50];
    int i;
    int j;
    int count = 0;

    for (i = 0; i < 50; i = i + 1) {
        mark[i] = 1;
    }
    mark[0] = 0;
    mark[1] = 0;

    for (i = 2; i * i < 50; i = i + 1) {
        if (mark[i] == 1) {
            for (j = i * i; j < 50; j = j + i) {
                mark[j] = 0;
            }
        }
    }

    for (i = 0; i < 50; i = i + 1) {
        count = count + mark[i];
    }
    return count;
}
