/* An array of structs, sorted by score, with string fields and a struct
   passed by value versus by pointer. */
#include <stdio.h>
#include <string.h>

struct Student {
    char name[16];
    int  score;
    float gpa;
};

void sort_by_score(struct Student s[], int n) {
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - 1 - i; j++)
            if (s[j].score < s[j + 1].score) {
                struct Student t = s[j];
                s[j] = s[j + 1];
                s[j + 1] = t;
            }
}

void curve(struct Student *s, int bonus) { s->score += bonus; }
int doubled(struct Student s) { s.score *= 2; return s.score; }

int main() {
    struct Student cls[4] = {
        {"Asha", 71, 3.1}, {"Ravi", 88, 3.7}, {"Meera", 64, 2.9}, {"Kabir", 93, 3.9}
    };
    curve(&cls[2], 10);
    printf("doubled: %d (original stays %d)\n", doubled(cls[0]), cls[0].score);
    sort_by_score(cls, 4);
    for (int i = 0; i < 4; i++)
        printf("%-6s %3d  %.1f\n", cls[i].name, cls[i].score, cls[i].gpa);
    printf("top: %s, name length %d\n", cls[0].name, (int)strlen(cls[0].name));
    return 0;
}
