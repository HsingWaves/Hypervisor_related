#ifndef CIRCULAR_LINKED_LIST_H
#define CIRCULAR_LINKED_LIST_H


typedef struct Node {
    int id;          
    struct Node* next; 
} Node;

Node* create_circular_list(int n);

void free_list(Node* head);

#endif  // CIRCULAR_LINKED_LIST_H
