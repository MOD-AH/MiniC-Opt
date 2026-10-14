/* A singly linked list on the heap: push, reverse in place, sum, free. */
#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

Node *push(Node *head, int v) {
    Node *n = (Node *)malloc(sizeof(Node));
    n->value = v;
    n->next = head;
    return n;
}

Node *reverse(Node *head) {
    Node *prev = NULL;
    while (head != NULL) {
        Node *next = head->next;
        head->next = prev;
        prev = head;
        head = next;
    }
    return prev;
}

void print_list(Node *p) {
    while (p) {
        printf("%d%s", p->value, p->next ? " -> " : "\n");
        p = p->next;
    }
}

int main(void) {
    Node *list = NULL;
    for (int i = 1; i <= 5; i++) list = push(list, i * i);
    print_list(list);
    list = reverse(list);
    print_list(list);
    int sum = 0;
    for (Node *p = list; p; p = p->next) sum += p->value;
    printf("sum = %d\n", sum);
    while (list) { Node *n = list->next; free(list); list = n; }
    return 0;
}
