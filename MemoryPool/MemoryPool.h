#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H
#include <stdint.h>

#include "Block_List.h"
#include "Error.h"
#include "Func.h"
#include "Memory_config.h"
#if MEMPOOL_MULTI_TASK_SAFE
#include "FreeRTOS.h"
#include "semphr.h"
#endif
typedef struct MemoryPool MemoryPool;
typedef struct MemoryPoolOpt MemoryPoolOpt;

// 内存池操作函数表
struct MemoryPoolOpt {
    void* (*alloc)(MemoryPool* pool);
    ERRCODE (*free)(MemoryPool* pool, void* obj);
    ERRCODE (*destory)(MemoryPool** pool);
    void (*observer)(const MemoryPool* pool, char* msg, size_t len);
    ERRCODE (*check)(const MemoryPool* pool, char* msg, size_t len);
};

typedef struct Observer {
    // 当前已分配 block 数
    size_t used_block_count;
    // 历史最大已分配 block 数
    size_t peak_used_block_count;
    // 分配失败次数
    size_t alloc_fail_count;
    // 重复释放次数
    size_t double_free_count;
    // 向前越界/头部破坏：magic 异常
    size_t header_corrupt_count;
    // 向后越界：破坏 canary
    size_t tail_overflow_count;
    // 野指针释放次数
    size_t wild_free_count;
} Observer;

struct MemoryPool {
    size_t block_size;       // 用户可用大小
    size_t block_num;        // block 数量
    size_t block_real_size;  // header + user + canary 后对齐大小
#if OBSERVER_MODE
    Observer observer;
#endif
#if MEMPOOL_MULTI_TASK_SAFE
    SemaphoreHandle_t lock;
#endif
    BlockListNode* freelist;  // 全局空闲链
    MemoryPoolOpt opt;
    char space[];  // 整块内存
};

/*
 * @brief 创建内存池
 * @param block_size 每个数据块的大小
 * @param count      内存池中 block 的总个数
 * @param pool       输出：内存池指针
 */
ERRCODE create_MemoryPool(size_t block_size, size_t count, MemoryPool** pool);

#define Create_MemoryPool(type, count, pool) \
    create_MemoryPool(sizeof(type), (count), (pool))
void* MemoryPool_Alloc(MemoryPool* pool);
ERRCODE MemoryPool_Free(MemoryPool* pool, void* obj);
ERRCODE MemoryPool_Destory(MemoryPool** pool);
ERRCODE MemoryPool_Check(const MemoryPool* pool, char* msg, size_t len);
void MemoryPool_Observer(const MemoryPool* pool, char* msg, size_t len);
#endif
