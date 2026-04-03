#include "xlist.h"

// 初始化链表
void StdListInit(PT_StdListDef list) {
    if (list != NULL) {
        list->head = NULL;
        list->tail = NULL;
        list->size = 0;
    }
}

// 在链表尾部插入节点
void StdListPushBack(PT_StdListDef list, PT_StdNodeDef node) {
    if (list == NULL || node == NULL) {
        return;
    }
    
    node->next = NULL;
    node->prev = list->tail;
    
    if (list->tail == NULL) {
        list->head = node;
    } else {
        list->tail->next = node;
    }
    
    list->tail = node;
    list->size++;
}

// 在链表头部插入节点
void StdListPushFront(PT_StdListDef list, PT_StdNodeDef node) {
    if (list == NULL || node == NULL) {
        return;
    }
    
    node->prev = NULL;
    node->next = list->head;
    
    if (list->head == NULL) {
        list->tail = node;
    } else {
        list->head->prev = node;
    }
    
    list->head = node;
    list->size++;
}

// 在指定节点后插入新节点
void StdListInsertAfter(PT_StdListDef list, PT_StdNodeDef prev_node, PT_StdNodeDef new_node) {
    if (list == NULL || prev_node == NULL || new_node == NULL) {
        return;
    }
    
    new_node->prev = prev_node;
    new_node->next = prev_node->next;
    
    if (prev_node->next == NULL) {
        list->tail = new_node;
    } else {
        prev_node->next->prev = new_node;
    }
    
    prev_node->next = new_node;
    list->size++;
}

// 从链表中移除节点
void StdListRemove(PT_StdListDef list, PT_StdNodeDef node) {
    if (list == NULL || node == NULL || list->size == 0) {
        return;
    }
    
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }
    
    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        list->tail = node->prev;
    }
    
    node->prev = NULL;
    node->next = NULL;
    
    list->size--;
}

// 获取链表头节点
PT_StdNodeDef StdListGetHeadNode(PT_StdListDef list) {
    return (list != NULL) ? list->head : NULL;
}

// 获取链表尾节点
PT_StdNodeDef StdListGetTailNode(PT_StdListDef list) {
    return (list != NULL) ? list->tail : NULL;
}

// 获取下一个节点
PT_StdNodeDef StdListGetNextNode(PT_StdNodeDef node) {
    return (node != NULL) ? node->next : NULL;
}

// 获取上一个节点
PT_StdNodeDef StdListGetPrevNode(PT_StdNodeDef node) {
    return (node != NULL) ? node->prev : NULL;
}

// 获取链表大小
int StdListGetSize(PT_StdListDef list) {
    return (list != NULL) ? list->size : 0;
}

// 检查链表是否为空
bool StdListIsEmpty(PT_StdListDef list) {
    return (list == NULL) || (list->size == 0);
}
