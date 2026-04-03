#include "share_mem_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>
#include <semaphore.h>

/**
 * 共享内存队列属性结构体
 */
typedef struct {
    const char* strId;      // 队列唯一标识符
    size_t elementSize;     // 每个元素大小(字节)
    size_t maxElements;     // 最大元素数量
    bool isCreate;          // 是否创建新队列(创建/打开)
} SMQ_Attributes;

/**
 * 共享内存队列控制块结构
 * 存储在共享内存的开头
 */
typedef struct {
    char strId[64];         // 队列唯一标识符
    size_t elementSize;     // 每个元素大小(字节)
    size_t maxElements;     // 最大元素数量
    size_t capacity;        // 队列容量(字节)
    volatile size_t writeIndex;  // 写索引
    volatile size_t readIndex;   // 读索引
    volatile size_t elementCount; // 当前元素数量
    volatile bool isValid;  // 队列是否有效
    pthread_mutex_t mutex;  // 互斥锁
    sem_t notEmpty;         // 非空信号量
    sem_t notFull;          // 非满信号量
    char data[0];           // 数据缓冲区起始地址
} SMQ_ControlBlock;

/**
 * 共享内存队列句柄内部结构
 */
typedef struct {
    SMQ_ControlBlock* pCtrlBlock;  // 指向共享内存中的控制块
    int shmFd;              // 共享内存文件描述符
    void* shmAddr;          // 共享内存映射地址
    size_t shmSize;         // 共享内存大小
    bool isCreator;         // 是否是创建者
} SMQ_InternalHandle;

// 静态函数声明
static key_t generate_key(const char* strId);
static size_t calculate_shm_size(size_t elementSize, size_t maxElements);
static E_StateCode init_sync_primitives(SMQ_ControlBlock* pCtrlBlock, bool isCreate);
static void destroy_sync_primitives(SMQ_ControlBlock* pCtrlBlock);

/**
 * 生成共享内存键值
 */
static key_t generate_key(const char* strId) {
    if (!strId || strlen(strId) == 0) {
        return -1;
    }
    
    // 使用字符串ID和固定项目ID生成键值
    return ftok(strId, 0x4150); // 0x4150 是自定义项目ID (AP)
}

/**
 * 计算共享内存大小
 */
static size_t calculate_shm_size(size_t elementSize, size_t maxElements) {
    return sizeof(SMQ_ControlBlock) + (elementSize * maxElements);
}

/**
 * 初始化同步原语
 */
static E_StateCode init_sync_primitives(SMQ_ControlBlock* pCtrlBlock, bool isCreate) {
    if (!pCtrlBlock) {
        return STATE_CODE_INVALID_PARAM;
    }
    
    int ret;
    
    // 初始化互斥锁
    pthread_mutexattr_t mutexAttr;
    ret = pthread_mutexattr_init(&mutexAttr);
    if (ret != 0) {
        syslog("Failed to initialize mutex attributes: %s", strerror(ret));
        return STATE_CODE_INIT_FAILURE;
    }
    
    // 设置互斥锁为进程间共享
    ret = pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    if (ret != 0) {
        syslog("Failed to set mutex pshared: %s", strerror(ret));
        pthread_mutexattr_destroy(&mutexAttr);
        return STATE_CODE_INIT_FAILURE;
    }
    
    if (isCreate) {
        // 创建者初始化互斥锁
        ret = pthread_mutex_init(&pCtrlBlock->mutex, &mutexAttr);
        if (ret != 0) {
            syslog("Failed to initialize mutex: %s", strerror(ret));
            pthread_mutexattr_destroy(&mutexAttr);
            return STATE_CODE_INIT_FAILURE;
        }
        
        // 初始化非空信号量 (初始值为0)
        ret = sem_init(&pCtrlBlock->notEmpty, 1, 0);
        if (ret != 0) {
            syslog("Failed to initialize notEmpty semaphore: %s", strerror(ret));
            pthread_mutex_destroy(&pCtrlBlock->mutex);
            pthread_mutexattr_destroy(&mutexAttr);
            return STATE_CODE_INIT_FAILURE;
        }
        
        // 初始化非满信号量 (初始值为最大元素数)
        ret = sem_init(&pCtrlBlock->notFull, 1, pCtrlBlock->maxElements);
        if (ret != 0) {
            syslog("Failed to initialize notFull semaphore: %s", strerror(ret));
            sem_destroy(&pCtrlBlock->notEmpty);
            pthread_mutex_destroy(&pCtrlBlock->mutex);
            pthread_mutexattr_destroy(&mutexAttr);
            return STATE_CODE_INIT_FAILURE;
        }
    }
    
    pthread_mutexattr_destroy(&mutexAttr);
    return STATE_CODE_NO_ERROR;
}

/**
 * 销毁同步原语
 */
static void destroy_sync_primitives(SMQ_ControlBlock* pCtrlBlock) {
    if (!pCtrlBlock) {
        return;
    }
    
    sem_destroy(&pCtrlBlock->notEmpty);
    sem_destroy(&pCtrlBlock->notFull);
    pthread_mutex_destroy(&pCtrlBlock->mutex);
}

/**
 * 创建或打开共享内存队列
 */
HANDLE ShareMemQue_Create(UINT32 uiFrameCount, UINT32 uiFrameSize, const char* strId) {
    SMQ_Attributes attr;
    attr.strId = strId;
    attr.elementSize = (size_t)uiFrameSize;
    attr.maxElements = (size_t)uiFrameCount;
    attr.isCreate = true; // 默认创建新队列，可以根据需要修改为打开现有队列
    if (!strId || uiFrameSize == 0 || uiFrameCount == 0) {
        syslog("Invalid parameters for ShareMemQue_Create");
        return NULL;
    }
    
    SMQ_InternalHandle* pHandle = (SMQ_InternalHandle*)malloc(sizeof(SMQ_InternalHandle));
    if (!pHandle) {
        syslog("Memory allocation failed for internal handle");
        return NULL;
    }
    memset(pHandle, 0, sizeof(SMQ_InternalHandle));
    
    key_t shmKey = generate_key(attr.strId);
    if (shmKey == -1) {
        syslog("Failed to generate shared memory key");
        free(pHandle);
        return NULL;
    }
    
    pHandle->shmSize = calculate_shm_size(attr.elementSize, attr.maxElements);
    pHandle->isCreator = attr.isCreate;
    
    // 创建或打开共享内存
    if (attr.isCreate) {
        pHandle->shmFd = shmget(shmKey, pHandle->shmSize, IPC_CREAT | IPC_EXCL | 0666);
        if (pHandle->shmFd == -1) {
            if (errno == EEXIST) {
                syslog("Shared memory already exists, trying to open it");
                pHandle->shmFd = shmget(shmKey, pHandle->shmSize, 0666);
                pHandle->isCreator = false;
            } else {
                syslog("Failed to create shared memory: %s", strerror(errno));
                free(pHandle);
                return NULL;
            }
        }
    } else {
        pHandle->shmFd = shmget(shmKey, pHandle->shmSize, 0666);
        if (pHandle->shmFd == -1) {
            syslog("Failed to open shared memory: %s", strerror(errno));
            free(pHandle);
            return NULL;
        }
    }
    
    // 映射共享内存
    pHandle->shmAddr = shmat(pHandle->shmFd, NULL, 0);
    if (pHandle->shmAddr == (void*)-1) {
        syslog("Failed to attach shared memory: %s", strerror(errno));
        shmctl(pHandle->shmFd, IPC_RMID, NULL);
        /* shmFd 是 shmget 返回的 shmid，不是文件描述符，不需要 close() */
        free(pHandle);
        return NULL;
    }
    
    pHandle->pCtrlBlock = (SMQ_ControlBlock*)pHandle->shmAddr;
    
    // 如果是创建者，初始化控制块
    if (pHandle->isCreator) {
        memset(pHandle->pCtrlBlock, 0, sizeof(SMQ_ControlBlock));
        strncpy(pHandle->pCtrlBlock->strId, attr.strId, sizeof(pHandle->pCtrlBlock->strId) - 1);
        pHandle->pCtrlBlock->elementSize = attr.elementSize;
        pHandle->pCtrlBlock->maxElements = attr.maxElements;
        pHandle->pCtrlBlock->capacity = pHandle->shmSize - sizeof(SMQ_ControlBlock);
        pHandle->pCtrlBlock->writeIndex = 0;
        pHandle->pCtrlBlock->readIndex = 0;
        pHandle->pCtrlBlock->elementCount = 0;
        pHandle->pCtrlBlock->isValid = true;
        
        // 初始化同步原语
        E_StateCode initResult = init_sync_primitives(pHandle->pCtrlBlock, true);
        if (initResult != STATE_CODE_NO_ERROR) {
            syslog("Failed to initialize synchronization primitives");
            shmdt(pHandle->shmAddr);
            shmctl(pHandle->shmFd, IPC_RMID, NULL);
            close(pHandle->shmFd);
            free(pHandle);
            return NULL;
        }
        
        syslog("Created shared memory queue: %s (size: %zu bytes, elements: %zu x %zu bytes)",
                attr.strId, pHandle->shmSize, attr.maxElements, attr.elementSize);
    } else {
        // 验证现有共享内存的有效性
        if (!pHandle->pCtrlBlock->isValid || 
            strcmp(pHandle->pCtrlBlock->strId, attr.strId) != 0 ||
            pHandle->pCtrlBlock->elementSize != attr.elementSize ||
            pHandle->pCtrlBlock->maxElements != attr.maxElements) {
            syslog("Shared memory queue validation failed - incompatible parameters");
            shmdt(pHandle->shmAddr);
            close(pHandle->shmFd);
            free(pHandle);
            return NULL;
        }
        
        // 打开者初始化同步原语
        E_StateCode initResult = init_sync_primitives(pHandle->pCtrlBlock, false);
        if (initResult != STATE_CODE_NO_ERROR) {
            syslog("Failed to initialize synchronization primitives for existing queue");
            shmdt(pHandle->shmAddr);
            close(pHandle->shmFd);
            free(pHandle);
            return NULL;
        }
        
        syslog("Opened existing shared memory queue: %s", attr.strId);
    }
    
    return (HANDLE)pHandle;
}

/**
 * 删除共享内存队列
 */
E_StateCode ShareMemQue_Delete(HANDLE handle) {
    if (!handle) {
        syslog("Invalid handle for ShareMemQue_Delete");
        return STATE_CODE_INVALID_PARAM;
    }
    
    SMQ_InternalHandle* pHandle = (SMQ_InternalHandle*)handle;
    E_StateCode result = STATE_CODE_NO_ERROR;
    
    if (pHandle->pCtrlBlock) {
        // 标记队列无效
        pthread_mutex_lock(&pHandle->pCtrlBlock->mutex);
        pHandle->pCtrlBlock->isValid = false;
        pthread_mutex_unlock(&pHandle->pCtrlBlock->mutex);
        
        // 如果是创建者，销毁同步原语
        if (pHandle->isCreator) {
            destroy_sync_primitives(pHandle->pCtrlBlock);
        }
    }
    
    // 解除共享内存映射
    if (pHandle->shmAddr) {
        if (shmdt(pHandle->shmAddr) == -1) {
            syslog("Failed to detach shared memory: %s", strerror(errno));
            result = STATE_CODE_INIT_FAILURE;
        }
    }
    
    // 删除共享内存 (仅创建者)
    if (pHandle->shmFd != -1 && pHandle->isCreator) {
        if (shmctl(pHandle->shmFd, IPC_RMID, NULL) == -1) {
            syslog("Failed to remove shared memory: %s", strerror(errno));
            result = STATE_CODE_INIT_FAILURE;
        }
        /* shmFd 是 shmid，不是文件描述符，不需要 close() */
    }
    
    syslog("Deleted shared memory queue: %s", pHandle->pCtrlBlock ? pHandle->pCtrlBlock->strId : "unknown");
    
    free(pHandle);
    return result;
}

/**
 * 通过ID获取共享内存队列句柄
 */
HANDLE ShareMemQue_GetID(const char* strId) {
    if (!strId) {
        syslog("Invalid strId for ShareMemQue_GetID");
        return NULL;
    }
    
    key_t shmKey = generate_key(strId);
    if (shmKey == -1) {
        syslog("Failed to generate shared memory key");
        return NULL;
    }
    
    /* 以只读方式探测并获取已有共享内存的元信息 */
    int shmFd = shmget(shmKey, 0, 0666);
    if (shmFd == -1) {
        syslog("Shared memory queue not found: %s", strId);
        return NULL;
    }
    
    void* shmAddr = shmat(shmFd, NULL, SHM_RDONLY);
    if (shmAddr == (void*)-1) {
        syslog("Failed to attach shared memory for info: %s", strerror(errno));
        return NULL;
    }
    
    SMQ_ControlBlock* pCtrlBlock = (SMQ_ControlBlock*)shmAddr;
    size_t elementSize = pCtrlBlock->elementSize;
    size_t maxElements = pCtrlBlock->maxElements;
    shmdt(shmAddr);
    
    /* 直接以打开者身份附加，不创建新共享内存 */
    SMQ_InternalHandle* pHandle = (SMQ_InternalHandle*)malloc(sizeof(SMQ_InternalHandle));
    if (!pHandle) {
        syslog("Memory allocation failed for internal handle");
        return NULL;
    }
    memset(pHandle, 0, sizeof(SMQ_InternalHandle));
    
    pHandle->shmSize = calculate_shm_size(elementSize, maxElements);
    pHandle->isCreator = false;
    pHandle->shmFd = shmFd;
    
    pHandle->shmAddr = shmat(shmFd, NULL, 0);
    if (pHandle->shmAddr == (void*)-1) {
        syslog("Failed to attach shared memory: %s", strerror(errno));
        free(pHandle);
        return NULL;
    }
    
    pHandle->pCtrlBlock = (SMQ_ControlBlock*)pHandle->shmAddr;
    
    if (!pHandle->pCtrlBlock->isValid ||
        strcmp(pHandle->pCtrlBlock->strId, strId) != 0 ||
        pHandle->pCtrlBlock->elementSize != elementSize ||
        pHandle->pCtrlBlock->maxElements != maxElements) {
        syslog("Shared memory queue validation failed");
        shmdt(pHandle->shmAddr);
        free(pHandle);
        return NULL;
    }
    
    E_StateCode initResult = init_sync_primitives(pHandle->pCtrlBlock, false);
    if (initResult != STATE_CODE_NO_ERROR) {
        syslog("Failed to initialize synchronization primitives for existing queue");
        shmdt(pHandle->shmAddr);
        free(pHandle);
        return NULL;
    }
    
    syslog("Opened existing shared memory queue: %s", strId);
    return (HANDLE)pHandle;
}

/**
 * 获取写入指针（不持锁返回，调用方在 PutWritePtr 前直接写入数据即可）
 */
void* ShareMemQue_GetWritePtr(HANDLE handle) {
    if (!handle) {
        syslog("Invalid handle for ShareMemQue_GetWritePtr");
        return NULL;
    }
    
    SMQ_InternalHandle* pHandle = (SMQ_InternalHandle*)handle;
    if (!pHandle->pCtrlBlock || !pHandle->pCtrlBlock->isValid) {
        syslog("Shared memory queue is invalid or not initialized");
        return NULL;
    }
    
    SMQ_ControlBlock* pCtrlBlock = pHandle->pCtrlBlock;
    
    /* 等待队列非满 */
    if (sem_wait(&pCtrlBlock->notFull) == -1) {
        syslog("Failed to wait on notFull semaphore: %s", strerror(errno));
        return NULL;
    }
    
    /* 加锁读取写索引后立即解锁，返回写入位置供调用方填充 */
    if (pthread_mutex_lock(&pCtrlBlock->mutex) != 0) {
        syslog("Failed to lock mutex for write operation");
        sem_post(&pCtrlBlock->notFull);
        return NULL;
    }
    size_t writePos = pCtrlBlock->writeIndex * pCtrlBlock->elementSize;
    void* pWritePtr = (void*)(pCtrlBlock->data + writePos);
    pthread_mutex_unlock(&pCtrlBlock->mutex);

    syslog("Got write pointer at element %zu", pCtrlBlock->writeIndex);
    return pWritePtr;
}

/**
 * 提交写入（元素入队）
 */
E_StateCode ShareMemQue_PutWritePtr(HANDLE handle) {
    if (!handle) {
        syslog("Invalid handle for ShareMemQue_PutWritePtr");
        return STATE_CODE_INVALID_PARAM;
    }
    
    SMQ_InternalHandle* pHandle = (SMQ_InternalHandle*)handle;
    if (!pHandle->pCtrlBlock || !pHandle->pCtrlBlock->isValid) {
        syslog("Shared memory queue is invalid or not initialized");
        return STATE_CODE_INVALID_PARAM;
    }
    
    SMQ_ControlBlock* pCtrlBlock = pHandle->pCtrlBlock;
    
    if (pthread_mutex_lock(&pCtrlBlock->mutex) != 0) {
        syslog("Failed to lock mutex for PutWritePtr");
        return STATE_CODE_INIT_FAILURE;
    }
    pCtrlBlock->writeIndex = (pCtrlBlock->writeIndex + 1) % pCtrlBlock->maxElements;
    pCtrlBlock->elementCount++;
    pthread_mutex_unlock(&pCtrlBlock->mutex);
    
    if (sem_post(&pCtrlBlock->notEmpty) == -1) {
        syslog("Failed to post notEmpty semaphore: %s", strerror(errno));
        return STATE_CODE_INIT_FAILURE;
    }
    
    return STATE_CODE_NO_ERROR;
}

/**
 * 获取读取指针（不持锁返回，调用方读取数据后调用 PutReadPtr 提交）
 */
void* ShareMemQue_GetReadPtr(HANDLE handle) {
    if (!handle) {
        syslog("Invalid handle for ShareMemQue_GetReadPtr");
        return NULL;
    }
    
    SMQ_InternalHandle* pHandle = (SMQ_InternalHandle*)handle;
    if (!pHandle->pCtrlBlock || !pHandle->pCtrlBlock->isValid) {
        syslog("Shared memory queue is invalid or not initialized");
        return NULL;
    }
    
    SMQ_ControlBlock* pCtrlBlock = pHandle->pCtrlBlock;
    
    /* 等待队列非空 */
    if (sem_wait(&pCtrlBlock->notEmpty) == -1) {
        syslog("Failed to wait on notEmpty semaphore: %s", strerror(errno));
        return NULL;
    }
    
    /* 加锁读取读索引后立即解锁，返回读取位置供调用方消费 */
    if (pthread_mutex_lock(&pCtrlBlock->mutex) != 0) {
        syslog("Failed to lock mutex for read operation");
        sem_post(&pCtrlBlock->notEmpty);
        return NULL;
    }
    size_t readPos = pCtrlBlock->readIndex * pCtrlBlock->elementSize;
    void* pReadPtr = (void*)(pCtrlBlock->data + readPos);
    pthread_mutex_unlock(&pCtrlBlock->mutex);

    syslog("Got read pointer at element %zu", pCtrlBlock->readIndex);
    return pReadPtr;
}

/**
 * 提交读取（元素出队）
 */
E_StateCode ShareMemQue_PutReadPtr(HANDLE handle) {
    if (!handle) {
        syslog("Invalid handle for ShareMemQue_PutReadPtr");
        return STATE_CODE_INVALID_PARAM;
    }
    
    SMQ_InternalHandle* pHandle = (SMQ_InternalHandle*)handle;
    if (!pHandle->pCtrlBlock || !pHandle->pCtrlBlock->isValid) {
        syslog("Shared memory queue is invalid or not initialized");
        return STATE_CODE_INVALID_PARAM;
    }
    
    SMQ_ControlBlock* pCtrlBlock = pHandle->pCtrlBlock;
    
    if (pthread_mutex_lock(&pCtrlBlock->mutex) != 0) {
        syslog("Failed to lock mutex for PutReadPtr");
        return STATE_CODE_INIT_FAILURE;
    }
    pCtrlBlock->readIndex = (pCtrlBlock->readIndex + 1) % pCtrlBlock->maxElements;
    pCtrlBlock->elementCount--;
    pthread_mutex_unlock(&pCtrlBlock->mutex);
    
    if (sem_post(&pCtrlBlock->notFull) == -1) {
        syslog("Failed to post notFull semaphore: %s", strerror(errno));
        return STATE_CODE_INIT_FAILURE;
    }
    
    return STATE_CODE_NO_ERROR;
}

/**
 * 获取队列当前元素数量
 */
int ShareMemQue_GetCount(HANDLE handle) {
    if (!handle) {
        syslog("Invalid handle for ShareMemQue_GetCount");
        return -1;
    }
    
    SMQ_InternalHandle* pHandle = (SMQ_InternalHandle*)handle;
    if (!pHandle->pCtrlBlock || !pHandle->pCtrlBlock->isValid) {
        syslog("Shared memory queue is invalid or not initialized");
        return -1;
    }
    
    return pHandle->pCtrlBlock->elementCount;
}
