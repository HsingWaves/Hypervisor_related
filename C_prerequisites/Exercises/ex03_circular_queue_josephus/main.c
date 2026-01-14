#include <stdio.h>
#include <stdlib.h>

#include "circular_linked_list.h"

static void josephus_problem(int n, int k, int m) {
    if (n <= 0 || k <= 0 || m <= 0) {
        printf("wrong parameters\n");
        return;
    }

    Node* head = create_circular_list(n);
    if (!head) {
        printf("\n");
        return;
    }

    Node* current = head;

    Node* prev = head;
    while (prev->next != head) prev = prev->next;

    for (int i = 1; i < k; ++i) {
        prev = current;
        current = current->next;
    }

    int first_printed = 0;

    while (current->next != current) {
        for (int step = 1; step < m; ++step) {
            prev = current;
            current = current->next;
        }

        if (first_printed) printf(" ");
        printf("%d", current->id);
        first_printed = 1;

        prev->next = current->next; // 
        Node* to_free = current;
        current = prev->next;       //
        free(to_free);
    }

    if (first_printed) printf(" ");
    printf("%d", current->id);
    free(current);

    printf("\n");
}

int main(void) {
    josephus_problem(5, 1, 2);  // 
    josephus_problem(7, 3, 1);  // 
    josephus_problem(9, 1, 8);  // 
    return 0;
}
