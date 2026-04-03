/***************************************************************************
 Copyright (C) 2007, Caton
 File name: ctos.c
 Description: Implement file of Caton OS wrapper Module
 	
 Date           Modification      		          Name        	
----          -----------------           -------				
07/11/23        1.0.0                              Scott
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "log.h"
#include "ctdef.h"
#include "ctos.h"
#include "pthread.h"

#define QUEUE_NB_MAX        50

typedef struct os_MsgSend_s
{
	struct os_MsgSend_s  *Next_p;
	void *Message_p;
} os_MsgSend_t;

typedef struct
{
	void *Memory_p;
	BOOL *Used_p;
	os_MsgSend_t *Pending_p;
	size_t ElementSize;
	unsigned int NoElements;
} os_MsgElem_t;

typedef struct os_MsgQueueList_s
{
	struct os_MsgQueueList_s *Next_p;
	g_msg_queue_t *MessageQueue_p;
	void  *Memory_p;
} os_MsgQueueList_t;

static os_MsgElem_t MessageArray[QUEUE_NB_MAX];
static BOOL MessageInitDone = FALSE;

static os_MsgQueueList_t *MessageQueueList_p = NULL;

static pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;

static void Init_Msg_Queues(void)
{
	os_MsgElem_t *Elem_p;
	int Index;

	if (MessageInitDone == FALSE) {
		MessageInitDone = TRUE;
		for (Index=0,Elem_p=MessageArray;Index<QUEUE_NB_MAX;Index++,Elem_p++) {
			Elem_p->Memory_p = NULL;
			Elem_p->Used_p = NULL;
			Elem_p->Pending_p = NULL;
			Elem_p->ElementSize = 0;
			Elem_p->NoElements = 0;
		}
	}
}


g_sem_t * os_semaphore_create(int value)
{
	int ret;
	g_sem_t *p;

	p=malloc(sizeof(g_sem_t));
	if (p == NULL)
		return NULL;
	ret = sem_init(&p->sem, 0, value);
	if (ret != 0) {
		free(p);
		return NULL;
	} else {
		p->flag = 1;
		return p;
	}

}

int os_semaphore_init(g_sem_t *sem, int value)
{
	int ret;
	
	if (sem == NULL)
		return -1;
	ret = sem_init(&sem->sem, 0, value);
	if (ret == 0) {
		sem->flag = 0;
		return 0;
	} else
		return -1;
}

int os_semaphore_delete(g_sem_t *sem)
{
	int ret;
	
	if (sem == NULL)
		return -1;
	ret = sem_destroy(&sem->sem);
	if (sem->flag == 1)
		free(sem);
	return (ret == 0 ? 0 : -1);
}

int os_semaphore_wait(g_sem_t *sem)
{
	int ret;

	if (sem == NULL)
		return -1;
	while ((ret = sem_wait(&sem->sem)) != 0) {
		if (errno != EINTR) {
			syslog("sem_wait error: %d\n", errno);
			break;
		}
	}
	return (ret == 0 ? 0 : -1);
}

int os_semaphore_wait_timeout(g_sem_t *sem, g_clock_t *timeout)
{
	struct timespec abstime;
	struct timeval  tval_timeout;
	int ret = -1; /* Timeout by default */
	
	if (sem == NULL)
		return -1;
	if (timeout == G_TIMEOUT_IMMEDIATE) {
		ret = sem_trywait(&sem->sem);
	} else if (timeout == G_TIMEOUT_INFINITY) {
		ret = os_semaphore_wait(sem);
	} else {
		tval_timeout = os_clock_to_time(*timeout);
    		abstime.tv_sec  = tval_timeout.tv_sec;
    		abstime.tv_nsec = ((long)tval_timeout.tv_usec*1000);
    		while ((ret = sem_timedwait(&sem->sem, &abstime)) != 0) {
				if (errno != EINTR) {
					break;
				}
    		}
	}

	return (ret == 0 ? 0 : -1);
}

int os_semaphore_signal(g_sem_t *sem)
{
	int ret;

	if (sem == NULL)
		return -1;
	ret = sem_post(&sem->sem);
	return (ret == 0 ? 0 : -1);
}

int os_semaphore_getvalue(g_sem_t *sem, int *val)
{

	int ret;

	if ((sem == NULL) || (val == NULL))
		return -1;
	ret = sem_getvalue(&sem->sem, val);
	return (ret == 0 ? 0 : -1);
}

g_mutex_t * os_mutex_create(void)
{
	int ret;
	g_mutex_t *p;
	pthread_mutexattr_t attr;

	p=malloc(sizeof(g_mutex_t));
	if (p == NULL)
		return NULL;
	ret = pthread_mutexattr_init(&attr);
	if (ret)
	{
		free(p);
		return NULL;
	}
	
	ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	if (ret) {
		pthread_mutexattr_destroy(&attr);
		free(p);
		return NULL;
	}
	
	ret = pthread_mutex_init(&p->mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	if (ret != 0) {
		free(p);
		return NULL;
	} else {
		p->flag = 1;
		return p;
	}
}

int os_mutex_init(g_mutex_t *mutex)
{
	int ret;
	pthread_mutexattr_t attr;
	
	if (mutex == NULL)
		return -1;
	ret = pthread_mutexattr_init(&attr);
	if (ret)
		return -1;
	ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	if (ret) {
		pthread_mutexattr_destroy(&attr);
		return -1;
	}
	
	ret = pthread_mutex_init(&mutex->mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	if (ret == 0)
	{
		mutex->flag = 0;
		return 0;
	}
	else
		return -1;
}

int os_mutex_delete(g_mutex_t *mutex)
{
	int ret;

	if (mutex == NULL)
		return -1;
	ret = pthread_mutex_destroy(&mutex->mutex);
	if (mutex->flag == 1)
		free(mutex);
	return (ret == 0 ? 0 : -1);
}


int os_mutex_lock(g_mutex_t *mutex)
{
	int ret;

	if (mutex == NULL)
		return -1;
	ret = pthread_mutex_lock(&mutex->mutex);
	return (ret == 0 ? 0 : -1);
}

int os_mutex_trylock(g_mutex_t *mutex)
{
	int ret;

	if (mutex == NULL)
		return -1;
	ret = pthread_mutex_trylock(&mutex->mutex);
	return (ret == 0 ? 0 : -1);
}

int os_mutex_lock_timeout_second(pthread_mutex_t *mutex, int times)
{
	int ret;
	struct timespec tout;
	
	if (mutex == NULL)
		return -1;

	clock_gettime(CLOCK_REALTIME, &tout);
	tout.tv_sec += times;

	ret = pthread_mutex_timedlock(mutex, &tout);
	return (ret == 0 ? 0 : -1);
}


int os_mutex_release(g_mutex_t *mutex)
{
	int ret;

	if (mutex == NULL)
		return -1;
	ret = pthread_mutex_unlock(&mutex->mutex);
	return (ret == 0 ? 0 : -1);
}

g_task_t  *os_task_create(void (*func)(void *), void *arg, size_t StackSize, int priority, char *name)
{
	pthread_attr_t  attr;
	g_task_t task, *task_p;
	int min_size;

	if (func == NULL)
		return NULL;
	min_size = sysconf(_SC_THREAD_STACK_MIN);
	if (min_size == -1)
		return NULL;
	
	pthread_attr_init(&attr);

	if (StackSize < (size_t)min_size)
		StackSize = min_size;
	pthread_attr_setstacksize(&attr, StackSize);
	if (priority >= 0) {
		struct sched_param sparam;

		if (priority < sched_get_priority_min(SCHED_RR))
			priority = sched_get_priority_min(SCHED_RR);
		if (priority > sched_get_priority_max(SCHED_RR))
			priority = sched_get_priority_max(SCHED_RR);

		sparam.sched_priority = priority;
		pthread_attr_setschedpolicy(&attr, SCHED_RR);
		pthread_attr_setschedparam(&attr, &sparam);
	}



	if (pthread_create(&task, &attr, (void*)func, arg) != 0) {
		pthread_attr_destroy(&attr);
		return NULL;
	}

	pthread_setname_np(task, name);

	task_p = malloc(sizeof(g_task_t));
	if (task_p) {
		*task_p = task;
	}
	pthread_attr_destroy(&attr);

	return task_p;
}

int os_task_wait(g_task_t  *task)
{
	int ret;

	if (task == NULL)
		return -1;
	ret = pthread_join(*task, NULL);
	return (ret == 0 ? 0 : -1);
}

int os_task_delete(g_task_t *task)
{
	if (task == NULL)
		return -1;
	free(task);
	return 0;
}

void os_sleep(g_clock_t delay)
{
	struct timespec x;
	int cps;

	if (delay < 0)
		return;
	cps = G_CLOCKS_PER_SECOND;
	/* if < 2ms */
	if (delay<= (2 * cps / 1000 )) {
		x.tv_sec  = 0;
		x.tv_nsec = 2000001;
	} else {
		x.tv_sec  = delay / cps;
		x.tv_nsec = (((delay % cps) * 1000000) / cps) * 1000;
	}
	nanosleep(&x, NULL);
}

g_clock_t os_ticks_per_sec(void)
{
	return G_CLOCKS_PER_SECOND;
}

g_msg_queue_t * os_message_create_queue_timeout(size_t ElementSize, unsigned int NoElements)
{
	os_MsgQueueList_t  *Current_p, *New_p;
	g_msg_queue_t *MesQ_p = NULL;
	os_MsgElem_t * Elem_p;
	int Index;

	if ((New_p = os_memory_allocate( sizeof(os_MsgQueueList_t))) != NULL) {
		if ((New_p->MessageQueue_p = os_memory_allocate( sizeof(g_msg_queue_t))) != NULL) {
			if ((New_p->Memory_p = os_memory_allocate( ElementSize*NoElements)) == NULL) {
				os_memory_deallocate( New_p->MessageQueue_p);
				os_memory_deallocate( New_p);
				New_p = NULL;
			}
		} else {
			os_memory_deallocate( New_p);
			New_p = NULL;
		}
	}

	if (New_p != NULL) {
		pthread_mutex_lock(&message_mutex);
		Init_Msg_Queues();
		Index = 0;
		Elem_p = MessageArray;
		while ((Index<QUEUE_NB_MAX) && (Elem_p->Memory_p != NULL)) {
			Index++;
			Elem_p++;
		}

		New_p->MessageQueue_p->Index = Index;

		if (Index < QUEUE_NB_MAX) {
			Elem_p = &MessageArray[ New_p->MessageQueue_p->Index ];
			Elem_p->Used_p = os_memory_allocate( NoElements*sizeof(BOOL));
			if (Elem_p->Used_p != NULL) {
				memset((void *)Elem_p->Used_p, 0, NoElements*sizeof(BOOL));
				Elem_p->Memory_p = New_p->Memory_p;
				Elem_p->Pending_p = NULL;
				Elem_p->ElementSize = ElementSize;
				Elem_p->NoElements = NoElements;

				New_p->MessageQueue_p->MsgSemaphore_p = os_semaphore_create(0);
				New_p->MessageQueue_p->ClaimSemaphore_p = os_semaphore_create(NoElements);
			}
		} else {
			syslog("Message queue is full, need more memory\n");
			pthread_mutex_unlock(&message_mutex);
			os_memory_deallocate( New_p->MessageQueue_p);
			os_memory_deallocate( New_p->Memory_p);
			os_memory_deallocate( New_p);
			return NULL;
		}
		pthread_mutex_unlock(&message_mutex);

		if (MessageArray[ New_p->MessageQueue_p->Index ].Used_p != NULL) {
			New_p->Next_p = NULL;
			pthread_mutex_lock(&message_mutex);
			Current_p = MessageQueueList_p;
			if (Current_p != NULL) {
				while (Current_p->Next_p != NULL)
					Current_p = Current_p->Next_p;
				
				Current_p->Next_p = New_p;
			} else {
				MessageQueueList_p = New_p;
			}
			pthread_mutex_unlock(&message_mutex);

			MesQ_p = New_p->MessageQueue_p;
		} else {
			os_memory_deallocate( New_p->MessageQueue_p);
			os_memory_deallocate( New_p->Memory_p);
			os_memory_deallocate( New_p);
			return NULL;
		}
	}

	return(MesQ_p);
}

g_msg_queue_t * os_message_create_queue(size_t ElementSize, unsigned int NoElements)
{
	return os_message_create_queue_timeout(ElementSize, NoElements);
}

void os_message_delete_queue2(g_msg_queue_t * MessageQueue, MsgFreeFunc func)
{
	os_MsgQueueList_t  * Current_p = NULL;
	os_MsgQueueList_t  * Deleted_p = NULL;
	os_MsgSend_t *Pending_p;
	os_MsgElem_t*Elem_p;

	if (MessageInitDone == FALSE) 
	{
		syslog("Message queue not initialize\n");
		return;
	}
	if (MessageQueue == NULL)
		return;

	if (MessageQueue->Index < QUEUE_NB_MAX) {
		Elem_p = &MessageArray[MessageQueue->Index];
		os_semaphore_delete(MessageQueue->MsgSemaphore_p);
		os_semaphore_delete(MessageQueue->ClaimSemaphore_p);

		pthread_mutex_lock(&message_mutex);

		Elem_p->Memory_p = NULL;
		Pending_p = Elem_p->Pending_p;
		while (Pending_p != NULL) {
			Elem_p->Pending_p = Elem_p->Pending_p->Next_p;
			if (NULL != func) {
				func((U32)NULL, Pending_p->Message_p);
			}
			os_memory_deallocate( Pending_p);
			Pending_p = Elem_p->Pending_p;
		}
		Elem_p->ElementSize = 0;
		Elem_p->NoElements = 0;
		if (Elem_p->Used_p != NULL) {
			os_memory_deallocate( Elem_p->Used_p);
			Elem_p->Used_p = NULL;
		}
		pthread_mutex_unlock(&message_mutex);
	}
	else {
		syslog("os_message_delete_queue( ) error\n");
		return;
	}
	MessageQueue->Index = QUEUE_NB_MAX;

	pthread_mutex_lock(&message_mutex);
	if (MessageQueueList_p != NULL) {
		if (MessageQueueList_p->MessageQueue_p == MessageQueue)
			Deleted_p = MessageQueueList_p;
		else {
			Current_p = MessageQueueList_p;
			while ((Current_p->Next_p != NULL) && (Deleted_p == NULL)) {
				if (Current_p->Next_p->MessageQueue_p == MessageQueue)
					Deleted_p = Current_p->Next_p;
				else
					Current_p = Current_p->Next_p;
			}
		}

		if (Deleted_p != NULL) {
			if (Deleted_p == MessageQueueList_p)
				MessageQueueList_p = Deleted_p->Next_p;
			else
				Current_p->Next_p = Deleted_p->Next_p;
			os_memory_deallocate( Deleted_p->Memory_p);
			os_memory_deallocate( Deleted_p->MessageQueue_p);
			os_memory_deallocate( Deleted_p);
		}
	}
	pthread_mutex_unlock(&message_mutex);
}

void os_message_delete_queue(g_msg_queue_t * MessageQueue)
{
	os_MsgQueueList_t  * Current_p = NULL;
	os_MsgQueueList_t  * Deleted_p = NULL;
	os_MsgSend_t *Pending_p;
	os_MsgElem_t*Elem_p;

	if (MessageInitDone == FALSE) {
		syslog("Message queue not initialize\n");
		return;
	}
	if (MessageQueue == NULL)
		return;

	if (MessageQueue->Index < QUEUE_NB_MAX) {
		Elem_p = &MessageArray[MessageQueue->Index];
		os_semaphore_delete(MessageQueue->MsgSemaphore_p);
		os_semaphore_delete(MessageQueue->ClaimSemaphore_p);

		pthread_mutex_lock(&message_mutex);

		Elem_p->Memory_p = NULL;
		Pending_p = Elem_p->Pending_p;
		while (Pending_p != NULL) {
			Elem_p->Pending_p = Elem_p->Pending_p->Next_p;
			os_memory_deallocate( Pending_p);
			Pending_p = Elem_p->Pending_p;
		}
		Elem_p->ElementSize = 0;
		Elem_p->NoElements = 0;
		if (Elem_p->Used_p != NULL) {
			os_memory_deallocate( Elem_p->Used_p);
			Elem_p->Used_p = NULL;
		}
		pthread_mutex_unlock(&message_mutex);
	}
	else {
		syslog("os_message_delete_queue( ) error\n");
		return;
	}
	MessageQueue->Index = QUEUE_NB_MAX;

	pthread_mutex_lock(&message_mutex);
	if (MessageQueueList_p != NULL) {
		if (MessageQueueList_p->MessageQueue_p == MessageQueue)
			Deleted_p = MessageQueueList_p;
		else {
			Current_p = MessageQueueList_p;
			while ((Current_p->Next_p != NULL) && (Deleted_p == NULL)) {
				if (Current_p->Next_p->MessageQueue_p == MessageQueue)
					Deleted_p = Current_p->Next_p;
				else
					Current_p = Current_p->Next_p;
			}
		}

		if (Deleted_p != NULL) {
			if (Deleted_p == MessageQueueList_p)
				MessageQueueList_p = Deleted_p->Next_p;
			else
				Current_p->Next_p = Deleted_p->Next_p;
			os_memory_deallocate( Deleted_p->Memory_p);
			os_memory_deallocate( Deleted_p->MessageQueue_p);
			os_memory_deallocate( Deleted_p);
		}
	}
	pthread_mutex_unlock(&message_mutex);
}

void * os_message_claim_timeout(g_msg_queue_t * MessageQueue, g_clock_t * time)
{
	os_MsgElem_t * Elem_p;
	unsigned int i;

	if (MessageQueue == NULL)
		return NULL;
	if (MessageQueue->Index < QUEUE_NB_MAX) {
		Elem_p = &MessageArray[MessageQueue->Index];

		if (os_semaphore_wait_timeout(MessageQueue->ClaimSemaphore_p, time) == 0) {
			pthread_mutex_lock(&message_mutex);
			for (i=0; i<Elem_p->NoElements; i++) {
				if (Elem_p->Used_p[i] == FALSE) {
					Elem_p->Used_p[i] = TRUE;
					pthread_mutex_unlock(&message_mutex);
					return (void *)((char *)Elem_p->Memory_p + i * Elem_p->ElementSize);
				}
			}
			pthread_mutex_unlock(&message_mutex);
		}
	}
	else {
		syslog("os_message_claim_timeout( ) error\n");
	}
	
	return NULL;
}

void * os_message_claim(g_msg_queue_t * MessageQueue)
{
	return os_message_claim_timeout(MessageQueue, G_TIMEOUT_INFINITY);
}

void os_message_send(g_msg_queue_t * MessageQueue, void * message)
{
	os_MsgSend_t  * Pending_p, * New_p;

	if ((MessageQueue == NULL) || (message == NULL))
		return;
	if (MessageQueue->Index < QUEUE_NB_MAX) {
		New_p = os_memory_allocate( sizeof(os_MsgSend_t));
		if (New_p != NULL) {
			New_p->Next_p = NULL;
			New_p->Message_p = message;
			pthread_mutex_lock(&message_mutex);
			Pending_p = MessageArray[MessageQueue->Index].Pending_p;
			if (Pending_p != NULL) {
				while (Pending_p->Next_p != NULL)
					Pending_p = Pending_p->Next_p;
				Pending_p->Next_p = New_p;
			} else
				MessageArray[ MessageQueue->Index ].Pending_p = New_p;
			pthread_mutex_unlock(&message_mutex);
			os_semaphore_signal(MessageQueue->MsgSemaphore_p);
		}
	}
	else {
		syslog("os_message_send( ) error\n");
	}
}

void * os_message_receive_timeout(g_msg_queue_t * MessageQueue, g_clock_t * time)
{
	os_MsgSend_t  * Pending_p;
	os_MsgElem_t  * Elem_p;
	void 		 * Message_p = NULL;
	int ret;
	
	if (MessageQueue == NULL)
		return NULL;
	if (MessageQueue->Index < QUEUE_NB_MAX) {
		if (os_semaphore_wait_timeout(MessageQueue->MsgSemaphore_p, time) == 0) {
			Elem_p = &MessageArray[MessageQueue->Index];
			
			ret = pthread_mutex_lock(&message_mutex);
			if(ret == 0)
			{
				Pending_p = Elem_p->Pending_p;
				if (Pending_p != NULL) {
					Elem_p->Pending_p = Pending_p->Next_p;
					Message_p = Pending_p->Message_p;
					os_memory_deallocate( Pending_p);
					pthread_mutex_unlock(&message_mutex);
					return Message_p;
				}
				pthread_mutex_unlock(&message_mutex);
			}

		}
	}
	else {
		syslog("os_message_receive_timeout( ) error\n");
	}

	return Message_p;
}

void * os_message_receive(g_msg_queue_t * MessageQueue)
{
	return os_message_receive_timeout(MessageQueue, G_TIMEOUT_INFINITY);
}

int os_message_query_num(g_msg_queue_t * MessageQueue)
{
	int num=0;
	if (MessageQueue == NULL)
		return 0;
	int ret = os_semaphore_getvalue(MessageQueue->MsgSemaphore_p, &num);
	if(ret != 0)
	{
		syslog("os_semaphore_getvalue failed!\n");
		return 0;
	}
	else
		return num;
}

void os_message_release(g_msg_queue_t * MessageQueue, void* Message)
{
	os_MsgElem_t * Elem_p;
	U32 Index;

	if ((MessageQueue == NULL) || (Message == NULL))
		return;
	if (MessageQueue->Index < QUEUE_NB_MAX) {
		Elem_p = &MessageArray[MessageQueue->Index];

		int ret = pthread_mutex_lock(&message_mutex);
		if(ret==0)
		{
			if (((U32)Message >= (U32)(Elem_p->Memory_p))
			&& ((U32)Message < (U32)(Elem_p->Memory_p) + Elem_p->ElementSize*Elem_p->NoElements)) {
			Index = ((U32)Message - (U32)(Elem_p->Memory_p))/((U32)(Elem_p->ElementSize));
			Elem_p->Used_p[Index] = FALSE;
			os_semaphore_signal(MessageQueue->ClaimSemaphore_p);
			}
			pthread_mutex_unlock(&message_mutex);
		}

	}
	else {
		syslog("os_message_release( ) error\n");
	}
}


void *os_memory_allocate(size_t size)
{
	return malloc(size);
}

void *os_memory_allocate_clear(size_t nelem, size_t size)
{
	return calloc(nelem, size);
}

void os_memory_deallocate(void *ptr)
{
	if (ptr)
		free(ptr);
}

void *os_memory_reallocate(void *ptr, size_t size)
{
	return realloc(ptr, size);
}


struct timeval os_clock_to_time(g_clock_t clock)
{
	struct timeval	tval;
	int usec_per_clk;

	if (gettimeofday(&tval, NULL) != 0) {
		tval.tv_sec  = 0;
		tval.tv_usec = 0;
		return tval;
	}
	usec_per_clk = 1000000/G_CLOCKS_PER_SECOND;
	tval.tv_sec  = (tval.tv_sec / (TIME_NOW_CONSTRAINTS-1))*(TIME_NOW_CONSTRAINTS-1)
		+ clock/G_CLOCKS_PER_SECOND;
	tval.tv_usec = (clock % G_CLOCKS_PER_SECOND)*usec_per_clk;

	return tval;
}


g_clock_t  os_time_now(void)
{
	struct timeval tv;
	g_clock_t clk = 1;
	long usec_per_clk;

	usec_per_clk = 1000000/G_CLOCKS_PER_SECOND;
	if (gettimeofday(&tv, NULL) == 0) {
		tv.tv_sec %= TIME_NOW_CONSTRAINTS - 1;
		clk =  tv.tv_sec*G_CLOCKS_PER_SECOND+ tv.tv_usec/usec_per_clk;
	}

	return clk;
}

int os_time_after(g_clock_t time1, g_clock_t time2)
{
	g_clock_t time;

	time = time1 - time2;
	if (time < 0) {
		time = -time;
		if (time < TIME_CLOCKS_MODULO / 2)
			return 0;
		else
			return 1;
	} else {
		if (time < TIME_CLOCKS_MODULO / 2)
			return 1;
		else
			return 0;
	}
}

g_clock_t os_time_minus(g_clock_t time1, g_clock_t time2)
{
	g_clock_t time;

	time = time1 - time2;
	if (time < 0) {
		if (os_time_after(time1, time2) == 0)
			return time;
		else
			return (TIME_CLOCKS_MODULO  + time);
	}
	else
		return time;
}

g_clock_t os_time_plus(g_clock_t time1, g_clock_t time2)
{
	return ((time1 + time2) % TIME_CLOCKS_MODULO);
}

void os_list_init(g_list_t *list)
{
	list->next = list;
	list->prev = list;
}
/*Insert a new entry after the specified head.
 * This is good for implementing stacks.*/
void os_list_add(g_list_t *new, g_list_t *head)
{	
	new->next = head->next;
	new->prev = head;
	head->next->prev = new;
	head->next = new;
}
/* Insert a new entry before the specified head.
 * This is useful for implementing queues.*/
void os_list_add_tail(g_list_t *new, g_list_t *head)
{	
	new->next = head;
	new->prev = head->prev;
	head->prev->next = new;
	head->prev = new;
}

void os_list_del(g_list_t *entry)
{
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
	entry->next = NULL;
	entry->prev = NULL;
}

int os_list_empty(g_list_t *head)
{
	return head->next == head;
}

void os_i64_set_value(U64 *Value,U32 MSW,U32 LSW)
{
	Value->MSW = MSW;
	Value->LSW = LSW;
}

BOOL os_i64_equal(U64 *Value1,U64 *Value2)
{
	if ((Value1->MSW==Value2->MSW) && (Value1->LSW==Value2->LSW))
		return TRUE;
	else
		return FALSE;
}

BOOL os_i64_greater(U64 *Value1,U64 *Value2)
{
	if (Value1->MSW>Value2->MSW)
		return TRUE;
	else if ((Value1->MSW==Value2->MSW) && (Value1->LSW>Value2->LSW))
		return TRUE; 
	else
		return FALSE;
}

BOOL os_i64_less(U64 *Value1,U64 *Value2)
{
	if (Value1->MSW<Value2->MSW)
		return TRUE;
	else if ((Value1->MSW==Value2->MSW) && (Value1->LSW<Value2->LSW))
		return TRUE; 
	else
		return FALSE;	
}

BOOL os_i64_greater_or_equal(U64 *Value1,U64 *Value2)
{
	if (Value1->MSW>Value2->MSW)
		return TRUE;
	else if ((Value1->MSW==Value2->MSW) && (Value1->LSW>=Value2->LSW))
		return TRUE; 
	else
		return FALSE;
}

BOOL os_i64_less_or_equal(U64 *Value1,U64 *Value2)
{
	if (Value1->MSW<Value2->MSW)
		return TRUE;
	else if ((Value1->MSW==Value2->MSW) && (Value1->LSW<=Value2->LSW))
		return TRUE; 
	else
		return FALSE;	
}

BOOL os_i64_is_zero(U64 *Value)
{
	if ((Value->MSW==0) && (Value->LSW==0))
		return TRUE;
	else
		return FALSE;
}

void os_i64_addU32(U64 *Result,U64 *Value1,U32 Value2)
{
	U64 V1=*Value1;
	U32 V2= Value2,Carry;
	
	Result->MSW = V1.MSW;
	Result->LSW = V1.LSW+V2;
	Carry = (((V1.LSW | V2) & 0x80000000) && (!(Result->LSW&0x80000000)));
	Carry |= (((V1.LSW & V2) & 0x80000000));
	if (Carry)
		Result->MSW++;
}

void os_i64_add(U64 *Result,U64 *Value1,U64 *Value2)
{
	unsigned long long V1,V2;

	V1=(((unsigned long long)Value1->MSW)<<32)+Value1->LSW;
	V2=(((unsigned long long)Value2->MSW)<<32)+Value2->LSW;
	V1=V1+V2;
	Result->MSW=(U32)(V1>>32);
	Result->LSW=(U32)V1;
}

void os_i64_subU32(U64 *Result,U64 *Value1,U32 Value2)
{
	U64 V1=*Value1;
	U32 V2= Value2;
	Result->MSW = V1.MSW-((V1.LSW<V2)?1:0);
	Result->LSW = V1.LSW-V2;
}

void os_i64_sub(U64 *Result,U64 *Value1,U64 *Value2)
{
	U64 V1=*Value1;
	U64 V2=*Value2;
	Result->MSW = V1.MSW-V2.MSW-((V1.LSW<V2.LSW)?1:0);
	Result->LSW = V1.LSW-V2.LSW;
}

void os_i64_mulU32(U64 *Result,U64 *Value1,U32 Value2)
{
	unsigned long long V1;
	U32 V2= Value2;

	V1=(((unsigned long long)Value1->MSW)<<32)+Value1->LSW;
	V1=V1*V2;
	Result->MSW=(U32)(V1>>32);
	Result->LSW=(U32)V1;
}

void os_i64_mul(U64 *Result,U64 *Value1,U64 *Value2)
{
	unsigned long long V1,V2;

	V1=(((unsigned long long)Value1->MSW)<<32)+Value1->LSW;
	V2=(((unsigned long long)Value2->MSW)<<32)+Value2->LSW;
	V1=V1*V2;
	Result->MSW=(U32)(V1>>32);
	Result->LSW=(U32)V1;
}

void os_i64_divU32(U64 *Result,U64 *Value1,U32 Value2)
{
	unsigned long long V1;
	U32 V2= Value2;

	if (V2==0)
	{
		Result->MSW=0;
		Result->LSW=0;
		return;
	}

	V1=(((unsigned long long)Value1->MSW)<<32)+Value1->LSW;
	V1=V1/V2;
	Result->MSW=(U32)(V1>>32);
	Result->LSW=(U32)V1;
}

void os_i64_div(U64 *Result,U64 *Value1,U64 *Value2)
{
	unsigned long long V1,V2;

	if ((Value2->LSW==0) && (Value2->MSW==0))
	{
		Result->MSW=0;
		Result->LSW=0;
		return;
	}

	V1=(((unsigned long long)Value1->MSW)<<32)+Value1->LSW;
	V2=(((unsigned long long)Value2->MSW)<<32)+Value2->LSW;
	V1=V1/V2;
	Result->MSW=(U32)(V1>>32);
	Result->LSW=(U32)V1;
}

void os_i64_shift_right(U64 *Result,U64 *Value1,U32 ShiftValue)
{
	U32 i;
	U64 V1=*Value1;

	if (ShiftValue == 0)
	{
		*Result = *Value1;
		return;
	}
	for (i=0;i<ShiftValue;i++)
	{
		Result->LSW=(V1.LSW>>1)&0x7FFFFFFF;
		if (V1.MSW&1)
			Result->LSW|=0x80000000;
		Result->MSW=(V1.MSW>>1)&0x7FFFFFFF;
		V1=*Result;
	}
}

void os_i64_shift_left(U64 *Result,U64 *Value1,U32 ShiftValue)
{
	U32 i;
	U64 V1=*Value1;

	if (ShiftValue == 0)
	{
		*Result = *Value1;
		return;
	}
	for (i=0;i<ShiftValue;i++)
	{
		Result->MSW=(V1.MSW<<1);
		if (V1.LSW&0x80000000)
			Result->MSW|=1;
		Result->LSW=(V1.LSW<<1);
		V1=*Result;
	}                
}

