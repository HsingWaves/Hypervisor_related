#include "circular_linked_list.h"

#include <stdlib.h>

Node* create_circular_list(int n) {
    if (n <= 0) return NULL;

    Node* head = (Node*)malloc(sizeof(Node));
    if (!head) return NULL;

    head->id = 1;
    head->next = head;  //

    Node* tail = head;

    for (int i = 2; i <= n; ++i) {
        Node* node = (Node*)malloc(sizeof(Node));
        if (!node) {
            // fail to allocate, free already created nodes
            free_list(head);
            return NULL;
        }
        node->id = i;
        node->next = head;     
        tail->next = node;     
        tail = node;           // update tail
    }

    return head;
}

void free_list(Node* head) {
    if (!head) return;

    // 
    Node* tail = head;
    while (tail->next != head) {
        tail = tail->next;
    }
    tail->next = NULL;

    Node* p = head;
    while (p) {
        Node* nxt = p->next;
        free(p);
        p = nxt;
    }
}
