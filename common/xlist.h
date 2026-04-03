#ifndef X_LIST_H
#define X_LIST_H

#include <stddef.h>
#include <stdbool.h>


// 链表节点定义
typedef struct T_StdNodeDef {
    struct T_StdNodeDef *next;  // 下一个节点
    struct T_StdNodeDef *prev;  // 上一个节点
} T_StdNodeDef, *PT_StdNodeDef;

// 链表管理结构体
typedef struct {
    PT_StdNodeDef head;  // 头节点
    PT_StdNodeDef tail;  // 尾节点
    int size;            // 链表元素数量
} T_StdListDef, *PT_StdListDef;

//! 强制用户将此宏放在结构体头部
//! 警告：必须作为结构体的第一个成员使用！
#define DECLARE_LIST_NODE_AT_HEAD() \
    T_StdNodeDef list_node

// 从数据指针转换为节点指针（仅当节点在头部时安全）
#define DATA_TO_NODE(data_ptr) \
    ((PT_StdNodeDef)(data_ptr))

// 从节点指针转换为数据指针（仅当节点在头部时安全）
#define NODE_TO_DATA(node_ptr, type) \
    ((type*)(node_ptr))

// 链表操作函数声明
void StdListInit(PT_StdListDef list);
void StdListPushBack(PT_StdListDef list, PT_StdNodeDef node);
void StdListPushFront(PT_StdListDef list, PT_StdNodeDef node);
void StdListInsertAfter(PT_StdListDef list, PT_StdNodeDef prev_node, PT_StdNodeDef new_node);
void StdListRemove(PT_StdListDef list, PT_StdNodeDef node);

PT_StdNodeDef StdListGetHeadNode(PT_StdListDef list);
PT_StdNodeDef StdListGetTailNode(PT_StdListDef list);
PT_StdNodeDef StdListGetNextNode(PT_StdNodeDef node);
PT_StdNodeDef StdListGetPrevNode(PT_StdNodeDef node);

int StdListGetSize(PT_StdListDef list);
bool StdListIsEmpty(PT_StdListDef list);


// 简化的链表操作宏
#define StdListPushBackData(list, data_ptr) \
    StdListPushBack(list, DATA_TO_NODE(data_ptr))

#define StdListPushFrontData(list, data_ptr) \
    StdListPushFront(list, DATA_TO_NODE(data_ptr))

#define StdListGetHeadData(list, type) \
    NODE_TO_DATA(StdListGetHeadNode(list), type)

#define StdListGetTailData(list, type) \
    NODE_TO_DATA(StdListGetTailNode(list), type)

#define StdListGetNextData(node_ptr, type) \
    NODE_TO_DATA(StdListGetNextNode(DATA_TO_NODE(node_ptr)), type)

#define StdListGetPrevData(node_ptr, type) \
    NODE_TO_DATA(StdListGetPrevNode(DATA_TO_NODE(node_ptr)), type)

#define StdListRemoveData(list, data_ptr) \
    StdListRemove(list, DATA_TO_NODE(data_ptr))

// 遍历宏
#define StdListForEachData(list, data_ptr, type) \
    for (data_ptr = StdListGetHeadData(list, type); \
         data_ptr != NULL; \
         data_ptr = StdListGetNextData(data_ptr, type))

#define StdListForEachDataSafe(list, data_ptr, next_ptr, type) \
    for (data_ptr = StdListGetHeadData(list, type), \
         next_ptr = (data_ptr ? StdListGetNextData(data_ptr, type) : NULL); \
         data_ptr != NULL; \
         data_ptr = next_ptr, \
         next_ptr = (data_ptr ? StdListGetNextData(data_ptr, type) : NULL))

#endif // X_LIST_H
