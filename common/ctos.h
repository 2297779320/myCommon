/**
 * @file ctos.h
 * @brief Caton OS 封装层 -- 信号量、互斥锁、任务、消息队列、内存、时钟、链表、I64 运算
 *
 * @details
 * 本文件是项目核心的 OS 抽象层之一（旧式 API，os_ 前缀），提供：
 *   - 信号量: g_sem_t + os_semaphore_create/wait/signal/delete
 *   - 互斥锁: g_mutex_t + os_mutex_create/lock/release/delete
 *   - 任务:   g_task_t + os_task_create/wait/delete
 *   - 消息队列: g_msg_queue_t + os_message_create/send/receive/release/delete
 *   - 内存:   os_memory_allocate/deallocate/reallocate
 *   - 时钟:   os_time_now/sleep/after/minus/plus
 *   - 链表:   g_list_t + os_list_init/add/del/empty
 *   - I64运算: os_i64_add/sub/mul/div/shift（内部已改用 uint64_t 实现）
 *
 * @note 与 osal.h 中的 T_MutexObj/T_SemaphoreObj 是两套独立的 OS 抽象，类型不可混用。
 *
 * @see ctdef.h（依赖 U32, U64, BOOL）
 * @see defs.h（依赖 pthread, sys/time 等系统头文件）
 * @see common.h（被依赖）
 */

#ifndef CTOS_H
#define CTOS_H


#include "ctdef.h"
#include "defs.h"


#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

/** @brief 信号量对象 */
typedef struct {
	sem_t sem;      /**< POSIX 信号量 */
	int flag;       /**< 1=由 create 分配需 free，0=由 init 初始化不需 free */
} g_sem_t;

/** @brief 互斥锁对象 */
typedef struct {
	pthread_mutex_t mutex;  /**< POSIX 互斥锁 */
	int flag;               /**< 1=由 create 分配需 free，0=由 init 初始化不需 free */
} g_mutex_t;

/** @brief 任务句柄类型 */
typedef pthread_t g_task_t;

/** @brief 消息队列对象 */
typedef struct {
	g_sem_t *MsgSemaphore_p;    /**< 消息可用信号量 */
	g_sem_t *ClaimSemaphore_p;  /**< 缓冲区可用信号量 */
	int Index;                  /**< 队列索引 */
} g_msg_queue_t;

typedef long g_clock_t;         /**< 时钟类型（毫秒） */
typedef int g_partition_t;      /**< 分区类型 */
typedef void (*MsgFreeFunc)(U32 handle, void *ptr); /**< 消息释放回调 */

#define G_TIMEOUT_INFINITY    ((g_clock_t  *)NULL)  /**< 永久等待 */
#define G_TIMEOUT_IMMEDIATE   ((g_clock_t *)-1)     /**< 立即返回 */
#define G_CLOCKS_PER_SECOND 1000                    /**< 每秒时钟数 */
#define TIME_NOW_CONSTRAINTS 864000                 /**< 时间约束 */
#define TIME_CLOCKS_MODULO (TIME_NOW_CONSTRAINTS*G_CLOCKS_PER_SECOND) /**< 时间模数 */

/** @brief 链表节点（侵入式，需嵌入到用户结构体中） */
typedef struct ctos_list {
	struct ctos_list *next, *prev; /**< 前后指针 */
} g_list_t;

/**
 * @brief 通过链表节点指针获取宿主结构体指针
 * @param[in] ptr    链表节点指针
 * @param[in] type   宿主结构体类型
 * @param[in] member 链表节点在宿主结构体中的成员名
 * @return 宿主结构体指针
 */
#define os_list_entry(ptr, type, member) \
	((type *)((char *)ptr - (size_t)(&((type *)0)->member)))

/**
 * @brief 遍历链表（仅获取节点指针）
 * @param[out] pos 循环变量，g_list_t* 类型
 * @param[in]  head 链表头节点
 */
#define os_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * @brief 遍历链表（获取宿主结构体指针）
 * @param[out] pos    循环变量，宿主结构体指针类型
 * @param[in]  head   链表头节点
 * @param[in] member  链表节点在宿主结构体中的成员名
 */
#define os_list_for_each_entry(pos, head, member) \
	for (pos = os_list_entry((head)->next, typeof(*pos), member);	\
		&pos->member != (head); 	\
		pos = os_list_entry(pos->member.next, typeof(*pos), member))

/**
 * @name 信号量函数
 * @{
 */
/** @brief 创建信号量 @param[in] value 初始值 @return 信号量指针，失败返回 NULL */
g_sem_t * os_semaphore_create(int value);
/** @brief 初始化已分配的信号量 @param[in,out] sem 信号量指针 @param[in] value 初始值 @return 0 成功 */
int os_semaphore_init(g_sem_t *sem, int value);
/** @brief 删除信号量 @param[in,out] sem 信号量指针 @return 0 成功 */
int os_semaphore_delete(g_sem_t *sem);
/** @brief 等待信号量（P 操作） @param[in,out] sem 信号量指针 @return 0 成功 */
int os_semaphore_wait(g_sem_t *sem);
/** @brief 带超时等待信号量 @param[in,out] sem 信号量指针 @param[in] timeout 超时时间 @return 0 成功，-1 超时 */
int os_semaphore_wait_timeout(g_sem_t *sem, g_clock_t *timeout);
/** @brief 释放信号量（V 操作） @param[in,out] sem 信号量指针 @return 0 成功 */
int os_semaphore_signal(g_sem_t *sem);
/** @brief 获取信号量当前值 @param[in] sem 信号量指针 @param[out] val 输出值 @return 0 成功 */
int os_semaphore_getvalue(g_sem_t *sem, int *val);
/** @} */

/**
 * @name 互斥锁函数
 * @{
 */
/** @brief 创建互斥锁 @return 互斥锁指针，失败返回 NULL */
g_mutex_t * os_mutex_create(void);
/** @brief 初始化已分配的互斥锁 @param[in,out] mutex 互斥锁指针 @return 0 成功 */
int os_mutex_init(g_mutex_t *mutex);
/** @brief 删除互斥锁 @param[in,out] mutex 互斥锁指针 @return 0 成功 */
int os_mutex_delete(g_mutex_t *mutex);
/** @brief 加锁 @param[in,out] mutex 互斥锁指针 @return 0 成功 */
int os_mutex_lock(g_mutex_t *mutex);
/** @brief 尝试加锁（非阻塞） @param[in,out] mutex 互斥锁指针 @return 0 成功，-1 已被占用 */
int os_mutex_trylock(g_mutex_t *mutex);
/**
 * @brief 带超时加锁
 * @param[in,out] mutex POSIX 互斥锁指针
 * @param[in] times 超时秒数
 * @return 0 成功，-1 超时
 */
int os_mutex_lock_timeout_second(pthread_mutex_t *mutex, int times);
/** @brief 解锁 @param[in,out] mutex 互斥锁指针 @return 0 成功 */
int os_mutex_release(g_mutex_t *mutex);
/** @} */

/**
 * @name 任务函数
 * @{
 */
/**
 * @brief 创建任务
 * @param[in] func 任务函数（void(*)(void*)）
 * @param[in] arg  传递给任务函数的参数
 * @param[in] StackSize 堆栈大小（字节）
 * @param[in] priority  优先级（-1 使用默认）
 * @param[in] name 任务名称
 * @return 任务句柄，失败返回 NULL
 */
g_task_t * os_task_create(void (*func)(void *), void *arg, size_t StackSize, int priority, char *name);
/** @brief 等待任务结束 @param[in,out] task 任务句柄 @return 0 成功 */
int os_task_wait(g_task_t *task);
/** @brief 删除任务（join 后释放句柄） @param[in,out] task 任务句柄 @return 0 成功 */
int os_task_delete(g_task_t *task);
/** @} */

/**
 * @name 消息队列函数
 * @{
 */
/**
 * @brief 创建消息队列（带超时）
 * @param[in] ElementSize 每个消息的大小（字节）
 * @param[in] NoElements  消息槽位数量
 * @return 消息队列指针，失败返回 NULL
 */
g_msg_queue_t * os_message_create_queue_timeout(size_t ElementSize, unsigned int NoElements);
/** @brief 创建消息队列（默认超时） @param[in] ElementSize 消息大小 @param[in] NoElements 槽数量 @return 队列指针 */
g_msg_queue_t * os_message_create_queue(size_t ElementSize, unsigned int NoElements);
/** @brief 查询队列中待处理消息数 @param[in] MessageQueue 队列指针 @return 消息数量 */
int os_message_query_num(g_msg_queue_t * MessageQueue);
/** @brief 删除消息队列 @param[in,out] MessageQueue 队列指针 */
void os_message_delete_queue(g_msg_queue_t * MessageQueue);
/** @brief 申请消息缓冲区（带超时） @param[in,out] MessageQueue 队列指针 @param[in] time 超时时间 @return 消息指针，超时返回 NULL */
void * os_message_claim_timeout(g_msg_queue_t * MessageQueue, g_clock_t * time);
/** @brief 申请消息缓冲区（永久等待） @param[in,out] MessageQueue 队列指针 @return 消息指针 */
void * os_message_claim(g_msg_queue_t * MessageQueue);
/** @brief 发送消息 @param[in,out] MessageQueue 队列指针 @param[in] message 消息指针 */
void os_message_send(g_msg_queue_t * MessageQueue, void * message);
/** @brief 接收消息（带超时） @param[in,out] MessageQueue 队列指针 @param[in] time 超时时间 @return 消息指针，超时返回 NULL */
void * os_message_receive_timeout(g_msg_queue_t * MessageQueue, g_clock_t * time);
/** @brief 接收消息（永久等待） @param[in,out] MessageQueue 队列指针 @return 消息指针 */
void * os_message_receive(g_msg_queue_t * MessageQueue);
/** @brief 释放消息缓冲区 @param[in,out] MessageQueue 队列指针 @param[in] Message 消息指针 */
void os_message_release(g_msg_queue_t * MessageQueue, void* Message);
/**
 * @brief 删除消息队列（带自定义释放回调）
 * @param[in,out] MessageQueue 队列指针
 * @param[in] func 消息释放回调，NULL 表示不回调
 */
void os_message_delete_queue2(g_msg_queue_t * MessageQueue, MsgFreeFunc func);
/** @} */

/**
 * @name 内存管理函数
 * @{
 */
/** @brief 分配内存 @param[in] size 字节数 @return 内存指针，失败返回 NULL */
void *os_memory_allocate(size_t size);
/** @brief 分配并清零内存 @param[in] nelem 元素数量 @param[in] size 每个元素大小 @return 内存指针 */
void *os_memory_allocate_clear(size_t nelem, size_t size);
/** @brief 释放内存 @param[in] ptr 内存指针 */
void os_memory_deallocate(void *ptr);
/** @brief 重新分配内存 @param[in] ptr 原内存指针 @param[in] size 新大小 @return 新内存指针 */
void *os_memory_reallocate(void *ptr, size_t size);
/** @} */

/**
 * @name 时钟函数
 * @{
 */
/** @brief 将时钟值转换为 timeval @param[in] clock 时钟值 @return timeval 结构体 */
struct timeval os_clock_to_time(g_clock_t clock);

/** @brief 获取当前时钟值（毫秒） @return 当前时钟 */
g_clock_t os_time_now(void);
/** @brief 判断 time1 是否在 time2 之后 @param[in] time1 @param[in] time2 @return 非0 表示是 */
int os_time_after(g_clock_t time1, g_clock_t time2);
/** @brief 计算 time1 - time2 @return 时间差 */
g_clock_t os_time_minus(g_clock_t time1, g_clock_t time2);
/** @brief 计算 time1 + time2 @return 时间和 */
g_clock_t os_time_plus(g_clock_t time1, g_clock_t time2);
/** @brief 休眠 @param[in] delay 休眠时长（毫秒） */
void os_sleep(g_clock_t delay);
/** @brief 获取每秒时钟数 @return 每秒时钟数（1000） */
g_clock_t os_ticks_per_sec(void);
/** @} */

/**
 * @name 链表函数
 * @{
 */
/** @brief 初始化链表头 @param[out] head 链表头 */
void os_list_init(g_list_t *head);
/** @brief 在链表头部插入节点 @param[in,out] new 新节点 @param[in,out] head 链表头 */
void os_list_add(g_list_t *new, g_list_t *head);
/** @brief 在链表尾部插入节点 @param[in,out] new 新节点 @param[in,out] head 链表头 */
void os_list_add_tail(g_list_t *new, g_list_t *head);
/** @brief 删除链表节点 @param[in,out] entry 要删除的节点 */
void os_list_del(g_list_t *entry);
/** @brief 检查链表是否为空 @param[in] head 链表头 @return 非0 表示空 */
int os_list_empty(g_list_t *head);
/** @} */

/**
 * @name I64 运算函数（基于 U64{MSW,LSW} 的 64 位运算）
 * @{
 */
/** @brief 设置 I64 值 @param[out] Result @param[in] MSW 高32位 @param[in] LSW 低32位 */
void os_i64_set_value(U64 *Value, U32 MSW, U32 LSW);
/** @brief 判断 Value1 == Value2 @return BOOL */
BOOL os_i64_equal(U64 *Value1, U64 *Value2);
/** @brief 判断 Value1 > Value2 @return BOOL */
BOOL os_i64_greater(U64 *Value1, U64 *Value2);
/** @brief 判断 Value1 < Value2 @return BOOL */
BOOL os_i64_less(U64 *Value1, U64 *Value2);
/** @brief 判断 Value1 >= Value2 @return BOOL */
BOOL os_i64_greater_or_equal(U64 *Value1, U64 *Value2);
/** @brief 判断 Value1 <= Value2 @return BOOL */
BOOL os_i64_less_or_equal(U64 *Value1, U64 *Value2);
/** @brief 判断是否为零 @return BOOL */
BOOL os_i64_is_zero(U64 *Value);
/** @brief I64 + U32 @param[out] Result @param[in] Value1 @param[in] Value2 */
void os_i64_addU32(U64 *Result, U64 *Value1, U32 Value2);
/** @brief I64 + I64 @param[out] Result @param[in] Value1 @param[in] Value2 */
void os_i64_add(U64 *Result, U64 *Value1, U64 *Value2);
/** @brief I64 - U32 @param[out] Result @param[in] Value1 @param[in] Value2 */
void os_i64_subU32(U64 *Result, U64 *Value1, U32 Value2);
/** @brief I64 - I64 @param[out] Result @param[in] Value1 @param[in] Value2 */
void os_i64_sub(U64 *Result, U64 *Value1, U64 *Value2);
/** @brief I64 * U32 @param[out] Result @param[in] Value1 @param[in] Value2 */
void os_i64_mulU32(U64 *Result, U64 *Value1, U32 Value2);
/** @brief I64 * I64 @param[out] Result @param[in] Value1 @param[in] Value2 */
void os_i64_mul(U64 *Result, U64 *Value1, U64 *Value2);
/** @brief I64 / U32 @param[out] Result @param[in] Value1 @param[in] Value2 */
void os_i64_divU32(U64 *Result, U64 *Value1, U32 Value2);
/** @brief I64 / I64 @param[out] Result @param[in] Value1 @param[in] Value2 */
void os_i64_div(U64 *Result, U64 *Value1, U64 *Value2);
/** @brief I64 右移 @param[out] Result @param[in] Value1 @param[in] ShiftValue 移位量（0-63） */
void os_i64_shift_right(U64 *Result, U64 *Value1, U32 ShiftValue);
/** @brief I64 左移 @param[out] Result @param[in] Value1 @param[in] ShiftValue 移位量（0-63） */
void os_i64_shift_left(U64 *Result, U64 *Value1, U32 ShiftValue);
/** @} */

#endif

