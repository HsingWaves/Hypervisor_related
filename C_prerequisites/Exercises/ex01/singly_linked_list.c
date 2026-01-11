#include "singly_linked_list.h"
#include <stdio.h>
#include <stdlib.h>

// 全局头指针（无额外头结点）
static link head = NULL;

// 创建新节点
link make_node(unsigned char item) {
    link p = (link)malloc(sizeof(*p));
    if (!p) {
        perror("malloc failed");
        exit(1);
    }
    p->item = item;
    p->next = NULL;
    return p;
}

// 释放节点
void free_node(link p) { free(p); }

// 查找节点（按 item 精确匹配）
link search(unsigned char key) {
    for (link cur = head; cur != NULL; cur = cur->next) {
        if (cur->item == key) return cur;
    }
    return NULL;
}

// 在链表头部插入节点（这里用“头插”，最简单/常用）
void insert(link p) {
    if (!p) return;
    p->next = head;
    head = p;
}

// 删除指定节点 p（p 必须是链表里的某个节点）
void delete_node(link p) {
    if (!p || !head) return;

    if (head == p) {
        head = head->next;
        free_node(p);
        return;
    }

    link prev = head;
    while (prev->next && prev->next != p) {
        prev = prev->next;
    }

    if (prev->next == p) {
        prev->next = p->next;
        free_node(p);
    }
    // 若没找到：什么也不做（防御性）
}

// 遍历链表
void traverse(void (*visit)(link)) {
    if (!visit) return;
    for (link cur = head; cur != NULL; cur = cur->next) {
        visit(cur);
    }
}

// 销毁整个链表
void destroy(void) {
    link cur = head;
    while (cur) {
        link nxt = cur->next;
        free_node(cur);
        cur = nxt;
    }
    head = NULL;
}

// 在链表头部推入节点
void push(link p) {
    insert(p);
}

// 从链表头部弹出节点（返回弹出的节点指针；调用者负责 free_node）
link pop(void) {
    if (!head) return NULL;
    link p = head;
    head = head->next;
    p->next = NULL;
    return p;
}

// 释放链表内存（释放从 list_head 开始的一条链）
void free_list(link list_head) {
    link cur = list_head;
    while (cur) {
        link nxt = cur->next;
        free_node(cur);
        cur = nxt;
    }
}
