/**
 * @file xlist.h
 * @brief 通用双向链表（叶子节点，无项目内依赖）
 *
 * @details
 * 提供侵入式双向链表实现，通过 DECLARE_LIST_NODE_AT_HEAD() 宏将链表节点
 * 嵌入到用户结构体头部，实现零额外内存分配的链表管理。
 *
 * @code
 * struct MyData { DECLARE_LIST_NODE_AT_HEAD(); int value; };
 * T_StdListDef list; StdListInit(&list);
 * struct MyData d = { .value = 42 };
 * StdListPushBack(&list, DATA_TO_NODE(&d));
 * @endcode
 *
 * @see common.h（被依赖）
 */

#ifndef X_LIST_H
#define X_LIST_H

#include <stddef.h>
#include <stdbool.h>


/** @brief 链表节点定义 */
typedef struct T_StdNodeDef {
    struct T_StdNodeDef *next;  /**< 下一个节点 */
    struct T_StdNodeDef *prev;  /**< 上一个节点 */
} T_StdNodeDef, *PT_StdNodeDef;

/** @brief 链表管理结构体 */
typedef struct {
    PT_StdNodeDef head;  /**< 头节点 */
    PT_StdNodeDef tail;  /**< 尾节点 */
    int size;            /**< 链表元素数量 */
} T_StdListDef, *PT_StdListDef;

/**
 * @brief 声明链表节点（必须放在结构体头部）
 * @warning 必须作为结构体的第一个成员使用！
 */
#define DECLARE_LIST_NODE_AT_HEAD() \
    T_StdNodeDef list_node

/** @brief 从数据指针转换为节点指针（仅当节点在头部时安全） */
#define DATA_TO_NODE(data_ptr) \
    ((PT_StdNodeDef)(data_ptr))

/** @brief 从节点指针转换为数据指针（仅当节点在头部时安全） */
#define NODE_TO_DATA(node_ptr, type) \
    ((type*)(node_ptr))

/** @brief 初始化链表 @param[out] list 链表管理结构体指针 */
void StdListInit(PT_StdListDef list);

/** @brief 在链表尾部插入节点 @param[in,out] list 链表 @param[in,out] node 要插入的节点 */
void StdListPushBack(PT_StdListDef list, PT_StdNodeDef node);

/** @brief 在链表头部插入节点 @param[in,out] list 链表 @param[in,out] node 要插入的节点 */
void StdListPushFront(PT_StdListDef list, PT_StdNodeDef node);

/**
 * @brief 在指定节点之后插入新节点
 * @param[in,out] list 链表
 * @param[in,out] prev_node 前驱节点
 * @param[in,out] new_node 要插入的节点
 */
void StdListInsertAfter(PT_StdListDef list, PT_StdNodeDef prev_node, PT_StdNodeDef new_node);

/** @brief 从链表中移除节点 @param[in,out] list 链表 @param[in,out] node 要移除的节点 */
void StdListRemove(PT_StdListDef list, PT_StdNodeDef node);

/** @brief 获取链表头节点 @param[in] list 链表 @return 头节点指针，空链表返回 NULL */
PT_StdNodeDef StdListGetHeadNode(PT_StdListDef list);

/** @brief 获取链表尾节点 @param[in] list 链表 @return 尾节点指针，空链表返回 NULL */
PT_StdNodeDef StdListGetTailNode(PT_StdListDef list);

/** @brief 获取下一个节点 @param[in] node 当前节点 @return 下一个节点指针 */
PT_StdNodeDef StdListGetNextNode(PT_StdNodeDef node);

/** @brief 获取上一个节点 @param[in] node 当前节点 @return 上一个节点指针 */
PT_StdNodeDef StdListGetPrevNode(PT_StdNodeDef node);

/** @brief 获取链表元素数量 @param[in] list 链表 @return 元素数量 */
int StdListGetSize(PT_StdListDef list);

/** @brief 检查链表是否为空 @param[in] list 链表 @return true 表示空 */
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
