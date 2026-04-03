
/* Define to prevent recursive inclusion */
#ifndef _CTOS_H_
#define _CTOS_H_


#include "ctdef.h"
#include "defs.h"


#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

typedef struct {
	sem_t sem;
	int flag;
} g_sem_t;

typedef struct {
	pthread_mutex_t mutex;
	int flag;
} g_mutex_t;
typedef pthread_t g_task_t;
typedef struct {
	g_sem_t *MsgSemaphore_p;
	g_sem_t *ClaimSemaphore_p;
	int Index;
} g_msg_queue_t;
typedef long g_clock_t;
typedef int g_partition_t;
typedef void (*MsgFreeFunc) (U32 handle, void *ptr);

#define G_TIMEOUT_INFINITY    ((g_clock_t  *)NULL)
#define G_TIMEOUT_IMMEDIATE   ((g_clock_t *)-1)
#define G_CLOCKS_PER_SECOND 1000
#define TIME_NOW_CONSTRAINTS 864000
#define TIME_CLOCKS_MODULO (TIME_NOW_CONSTRAINTS*G_CLOCKS_PER_SECOND)


typedef struct ctos_list {
	struct ctos_list *next, *prev;
} g_list_t;

/** 
 * os_list_entry - get the struct for this entry 
 * @ptr:	the &struct list_head pointer. 
 * @type:	the type of the struct this is embedded in. 
 * @member:	the name of the list_struct within the struct. 
 */
#define os_list_entry(ptr, type, member) \
	((type *)((char *)ptr - (int)(&((type *)0)->member)))

/** 
 * os_list_for_each	-	iterate over a list 
 * @pos:	the &struct list_head to use as a loop counter. 
 * @head:	the head for your list. 
 */
#define os_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)
/** 
 * os_list_for_each_entry	-	iterate over list of given type 
 * @pos:	the type * to use as a loop counter. 
 * @head:	the head for your list. 
 * @member:	the name of the list_struct within the struct. 
 */
#define os_list_for_each_entry(pos, head, member) \
	for (pos = os_list_entry((head)->next, typeof(*pos), member);	\
		&pos->member != (head); 	\
		pos = os_list_entry(pos->member.next, typeof(*pos), member))

/*******************************************************/
/*              Functions Declarations                 */
/*******************************************************/

/* semaphore functions */
g_sem_t * os_semaphore_create(int value);
int os_semaphore_init(g_sem_t *sem, int value);
int os_semaphore_delete(g_sem_t *sem);
int os_semaphore_wait(g_sem_t *sem);
int os_semaphore_wait_timeout(g_sem_t *sem, g_clock_t *timeout);
int os_semaphore_signal(g_sem_t *sem);
int os_semaphore_getvalue(g_sem_t *sem, int *val);

/* mutex functions */
g_mutex_t * os_mutex_create(void);
int os_mutex_init(g_mutex_t *mutex);
int os_mutex_delete(g_mutex_t *mutex);
int os_mutex_lock(g_mutex_t *mutex);
int os_mutex_trylock(g_mutex_t *mutex);
int os_mutex_lock_timeout_second(pthread_mutex_t *mutex, int times);
int os_mutex_release(g_mutex_t *mutex);

/* task functions */
g_task_t * os_task_create(void (*func)(void *), void *arg, size_t StackSize, int priority, char *name);
int os_task_wait(g_task_t *task);
int os_task_delete(g_task_t  *task);

/* Messages functions */
g_msg_queue_t * os_message_create_queue_timeout(size_t ElementSize, unsigned int NoElements);
g_msg_queue_t * os_message_create_queue(size_t ElementSize, unsigned int NoElements);
int os_message_query_num(g_msg_queue_t * MessageQueue);
void os_message_delete_queue(g_msg_queue_t * MessageQueue);
void * os_message_claim_timeout(g_msg_queue_t * MessageQueue, g_clock_t * time);
void * os_message_claim(g_msg_queue_t * MessageQueue);
void os_message_send(g_msg_queue_t * MessageQueue, void * message);
void * os_message_receive_timeout(g_msg_queue_t * MessageQueue, g_clock_t * time);
void * os_message_receive(g_msg_queue_t * MessageQueue);
void os_message_release(g_msg_queue_t * MessageQueue, void* Message);
void os_message_delete_queue2(g_msg_queue_t * MessageQueue, MsgFreeFunc func);

/* Memory Management */
void *os_memory_allocate( size_t size);
void *os_memory_allocate_clear( size_t nelem, size_t size);
void os_memory_deallocate( void *ptr);
void *os_memory_reallocate( void *ptr, size_t size);

/* Clock functions */
struct timeval os_clock_to_time(g_clock_t clock);

g_clock_t  os_time_now(void);
int os_time_after(g_clock_t time1, g_clock_t time2);
g_clock_t os_time_minus(g_clock_t time1, g_clock_t time2);
g_clock_t os_time_plus(g_clock_t time1, g_clock_t time2);
void os_sleep(g_clock_t delay);
g_clock_t os_ticks_per_sec(void);

/* List functions */
void os_list_init(g_list_t *head);
void os_list_add(g_list_t *new, g_list_t *head);
void os_list_add_tail(g_list_t *new, g_list_t *head);
void os_list_del(g_list_t *entry);
int os_list_empty(g_list_t *head);

void os_i64_set_value(U64 *Value,U32 MSW,U32 LSW);
BOOL os_i64_equal(U64 *Value1,U64 *Value2);
BOOL os_i64_greater(U64 *Value1,U64 *Value2);
BOOL os_i64_less(U64 *Value1,U64 *Value2);
BOOL os_i64_greater_or_equal(U64 *Value1,U64 *Value2);
BOOL os_i64_less_or_equal(U64 *Value1,U64 *Value2);
BOOL os_i64_is_zero(U64 *Value);
void os_i64_addU32(U64 *Result,U64 *Value1,U32 Value2);
void os_i64_add(U64 *Result,U64 *Value1,U64 *Value2);
void os_i64_subU32(U64 *Result,U64 *Value1,U32 Value2);
void os_i64_sub(U64 *Result,U64 *Value1,U64 *Value2);
void os_i64_mulU32(U64 *Result,U64 *Value1,U32 Value2);
void os_i64_mul(U64 *Result,U64 *Value1,U64 *Value2);
void os_i64_divU32(U64 *Result,U64 *Value1,U32 Value2);
void os_i64_div(U64 *Result,U64 *Value1,U64 *Value2);
void os_i64_shift_right(U64 *Result,U64 *Value1,U32 ShiftValue);
void os_i64_shift_left(U64 *Result,U64 *Value1,U32 ShiftValue);

#endif

