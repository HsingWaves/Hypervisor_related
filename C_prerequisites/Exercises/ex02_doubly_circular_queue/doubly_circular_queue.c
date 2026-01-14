#include "doubly_circular_queue.h"

#include <stdlib.h>

// 头尾哨兵（两个哨兵结点）
// 目标：形成“循环”的双向链表：head <-> ... <-> tail，并且 tail.next=head, head.prev=tail
static struct node headsentinel;
static struct node tailsentinel;

static link head = &headsentinel;
static link tail = &tailsentinel;

static void ensure_init(void) {
    static int inited = 0;
    if (inited) return;
    inited = 1;

    head->data = 0;
    tail->data = 0;

    head->next = tail;
    head->prev = tail;

    tail->prev = head;
    tail->next = head;
}

link make_node(int data) {
    ensure_init();
    link p = (link)malloc(sizeof(*p));
    if (!p) return NULL;
    p->data = data;
    p->prev = NULL;
    p->next = NULL;
    return p;
}

void free_node(link p) {
    ensure_init();
    if (!p) return;
    if (p == head || p == tail) return;
    free(p);
}

link search(int key) {
    ensure_init();
    for (link p = head->next; p != tail; p = p->next) {
        if (p->data == key) return p;
    }
    return NULL;
}

void insert(link p) {
    ensure_init();
    if (!p) return;

    p->next = head->next;
    p->prev = head;

    head->next->prev = p;
    head->next = p;
}

// delete_node: not free the node, just remove it from the list
void delete_node(link p) {
    ensure_init();
    if (!p) return;
    // not allow removing sentinels
    if (p == head || p == tail) return;

    p->prev->next = p->next;
    p->next->prev = p->prev;

    p->prev = NULL;
    p->next = NULL;
}

void traverse(void (*visit)(link)) {
    ensure_init();
    if (!visit) return;

    for (link p = head->next; p != tail; p = p->next) {
        visit(p);
    }
}

void destroy(void) {
    ensure_init();
    // 释放所有非哨兵结点
    link p = head->next;
    while (p != tail) {
        link nxt = p->next;
        free_node(p);
        p = nxt;
    }

    // 恢复为空队列结构
    head->next = tail;
    tail->prev = head;

    // 循环链接依旧保持
    head->prev = tail;
    tail->next = head;
}
