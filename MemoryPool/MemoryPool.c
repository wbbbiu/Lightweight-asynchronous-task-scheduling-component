
#include "MemoryPool.h"

#include <stdlib.h>
#include <string.h>

#include "Memory_config.h"
#define MEMPOOL_ALIGN_SIZE 4U
// 将这个大小对齐为 MEMPOOL_ALIGN_SIZE 的倍数
static inline size_t MemoryPool_AlignUp(size_t size) {
    return (size + MEMPOOL_ALIGN_SIZE - 1U) & ~(MEMPOOL_ALIGN_SIZE - 1U);
}
static MemoryPoolOpt opt = {.alloc = MemoryPool_Alloc,
                            .destory = MemoryPool_Destory,
                            .free = MemoryPool_Free,
                            .observer = MemoryPool_Observer,
                            .check = MemoryPool_Check};
void observer_init(Observer* observer, MemoryPool* pool) {
#if OBSERVER_MODE
    observer->used_block_count = 0;
    observer->peak_used_block_count = 0;
    observer->alloc_fail_count = 0;
    observer->double_free_count = 0;
    observer->header_corrupt_count = 0;
    observer->tail_overflow_count = 0;
    observer->wild_free_count = 0;
#else
    (void)observer;
    (void)pool;
#endif
}
// 初始化内存池的参数
void MemoryPool_init(MemoryPool* pool, size_t block_size, size_t count) {
    if (!pool) {
        return;
    }
    pool->block_size = block_size;
    pool->block_num = count;
    // 保证这个大小对齐为 MEMPOOL_ALIGN_SIZE 的倍数，否则可能触发硬件错误
    pool->block_real_size = MemoryPool_AlignUp(sizeof(BlockListNode) +
                                               pool->block_size + CANARY_SIZE);
    pool->freelist = NULL;
    pool->opt = opt;
#if OBSERVER_MODE
    observer_init(&pool->observer, pool);
#endif
}
// 检查内存池的参数是否合法
static inline ERRCODE Arg_Check(size_t block_size, size_t count,
                                MemoryPool** pool) {
    if (!pool) {
        return ERROR(ERR_PTR_NULL);
    }
    if (!block_size) {
        return ERROR(ERR_ARGS, "block不能为0");
    }
    if (count < 2) {
        return ERROR(ERR_ARGS, "count小于2没有意义");
    }
    return ERR_OK;
}
// 创建内存池之后需要初始化里面的块
void Block_Init(MemoryPool* pool) {
    uint32_t offset = 0;
    for (size_t i = 0; i < pool->block_num; i++) {
        BlockListNode* node = (BlockListNode*)(pool->space + offset);
        offset += pool->block_real_size;
        // 提供这个重复释放检查的功能
#if MEMPOOL_CHECK_FREE
        node->state = 1;  // free
#endif
// 提供这个向前越界检查和header损坏检查的功能
#if MEMPOOL_MAGIC
        node->magic = MAGIC;
#endif
// 提供这个向后越界检查和tail损坏检查的功能
#if MEMPOOL_CANARY
        uint32_t* canary = (uint32_t*)((uint8_t*)node + sizeof(BlockListNode) +
                                       pool->block_size);
        *canary = CANARY;
#endif
        BlockList_push(&pool->freelist, node);
    }
}

// 创建内存池
ERRCODE create_MemoryPool(size_t block_size, size_t count, MemoryPool** pool) {
    if (Arg_Check(block_size, count, pool) != ERR_OK) {
        return ERROR(ERR_ARGS);
    }

    // 初始化内存池空间,大小为 count 个 block_real_size 大小的内存块
    size_t block_real_size =
        MemoryPool_AlignUp(sizeof(BlockListNode) + block_size + CANARY_SIZE);
    size_t total_size = sizeof(MemoryPool) + block_real_size * count;
    *pool = (MemoryPool*)malloc(total_size);
    if (!*pool) {
        free(*pool);
        *pool = NULL;
        return ERROR(ERR_MEM);
    }
    MemoryPool_init(*pool, block_size, count);
    // 初始化内存池的块
    Block_Init(*pool);
    return ERR_OK;
}
static inline ERRCODE Block_Check(MemoryPool* pool, void* obj,
                                  BlockListNode** node) {
    if (!pool || !obj || !node) {
        return ERROR(ERR_PTR_NULL);
    }
    // 获取到这个指针所属的块节点
    *node = (BlockListNode*)((uint8_t*)obj - sizeof(BlockListNode));
// 检查这个指针是否是野指针
#if MEMPOOL_WILD_FREE_CHECK
    {
        uintptr_t addr = (uintptr_t)(*node);
        // 内存池数据起始地址
        uintptr_t start = (uintptr_t)pool->space;
        // 内存池数据结束地址
        uintptr_t end = start + pool->block_real_size * pool->block_num;
        // 检查这个指针是否在内存池的范围内
        if (addr < start || addr >= end) {
#if OBSERVER_MODE
            pool->observer.wild_free_count++;
#endif
            return ERROR(ERR_OPT, "当前指针不是本内存池指针");
        }
        // 因为每个块大小是一致的所以这个指针减去起始地址肯定是 block_real_size
        // 的倍数
        if (((addr - start) % pool->block_real_size) != 0) {
#if OBSERVER_MODE
            pool->observer.wild_free_count++;
#endif
            return ERROR(ERR_OPT, "当前指针不是合法block起始地址");
        }
    }
#endif
// 检查这个指针是否被重复释放
#if MEMPOOL_CHECK_FREE
    {
        // state 为 1 表示这个指针被释放了
        if ((*node)->state) {
#if OBSERVER_MODE
            pool->observer.double_free_count++;
#endif
            return ERROR(ERR_FREE_REPEAT, "存在地址被多次释放");
        }
    }
#endif
// 检查向前越界和这个头是否被破坏
#if MEMPOOL_MAGIC
    {
        if ((*node)->magic != MAGIC) {
#if OBSERVER_MODE
            pool->observer.header_corrupt_count++;
#endif
            return ERROR(ERR_BOUNDARY, "当前指针头部被破坏");
        }
    }
#endif
// 检查向后越界,但是注意padding部分的越界无法检查出来
#if MEMPOOL_CANARY
    {
        uint32_t* canary = (uint32_t*)((uint8_t*)obj + pool->block_size);
        if (*canary != CANARY) {
#if OBSERVER_MODE
            pool->observer.tail_overflow_count++;
#endif
            return ERROR(ERR_BOUNDARY, "当前指针尾部越界");
        }
    }
#endif
    return ERR_OK;
}

void* MemoryPool_Alloc(MemoryPool* pool) {
    if (!pool) {
        return NULL;
    }

    MEMPOOL_LOCK();

    // 从空闲块列表中获取一个块
    BlockListNode* node = BlockList_pop(&pool->freelist);
    // 获取失败计数增加
    if (!node) {
#if OBSERVER_MODE
        pool->observer.alloc_fail_count++;
#endif
        MEMPOOL_UNLOCK();
        return NULL;
    }
#if MEMPOOL_CHECK_FREE
    node->state = 0;
#endif
#if OBSERVER_MODE
    pool->observer.used_block_count++;
    pool->observer.peak_used_block_count = MAX(
        pool->observer.peak_used_block_count, pool->observer.used_block_count);
#endif

    MEMPOOL_UNLOCK();

    return (void*)((uint8_t*)node + sizeof(BlockListNode));
}
ERRCODE MemoryPool_Free(MemoryPool* pool, void* obj) {
    if (!pool || !obj) {
        return ERROR(ERR_PTR_NULL);
    }
    BlockListNode* node = NULL;
    // 先检查这个指针是否合法
    ERRCODE ret = Block_Check(pool, obj, &node);
    if (ret != ERR_OK) {
        return ret;
    }

#if MEMPOOL_CHECK_FREE
    node->state = 1;  // free
#endif

    MEMPOOL_LOCK();

    // 将这个块节点放回空闲块列表

    BlockList_push(&pool->freelist, node);

#if OBSERVER_MODE
    if (pool->observer.used_block_count > 0) {
        pool->observer.used_block_count--;
    }
#endif

    MEMPOOL_UNLOCK();

    return ERR_OK;
}
/*
 * @brief 检查内存池是否出现错误
 *
 * @param pool 内存池指针
 * @param msg 错误信息指针
 * @param len 错误信息长度
 * @return ERRCODE 错误码
 * @note 如果没有错误,则�返回 ERR_OK
 */
ERRCODE MemoryPool_Check(const MemoryPool* pool, char* msg, size_t len) {
    ERRCODE ret = ERR_OK;
    uint32_t header_corrupt = 0, tail_overflow = 0;
    if (!pool) {
        return ERROR(ERR_PTR_NULL);
    }
#if MEMPOOL_CANARY || MEMPOOL_MAGIC
    uint32_t offset = 0;
    // 依次检查每个块节点是否出现问题
    for (size_t i = 0; i < pool->block_num; i++) {
        BlockListNode* node = (BlockListNode*)(pool->space + i * offset);
        offset += pool->block_real_size;
        // 检查头部是否被破坏
#if MEMPOOL_MAGIC
        if (node->magic != MAGIC) {
            ret = ERR_BOUNDARY;
            header_corrupt++;
        }
#endif
        // 检查是否向后越界
#if MEMPOOL_CANARY
        uint32_t* canary = (uint32_t*)((uint8_t*)node + sizeof(BlockListNode) +
                                       pool->block_size);

        if (*canary != CANARY) {
            ret = ERR_BOUNDARY;
            tail_overflow++;
        }
#endif
    }
#endif
    sprintf(msg, "头部破坏次数:%d,尾部越界次数:%d", header_corrupt,
            tail_overflow);
    return ret;
}
/*
 * @brief 观察内存池状态
 *
 * @param pool 内存池指针
 * @param msg 观察信息指针
 * @param len 观察信息长度
 * @return void
 * @note
 * 观察内存池的状态,包括已分配block数,最大已分配block数,分配失败次数,非法释放次数,重复释放次数,头部破坏次数,尾部越界次数
 */
void MemoryPool_Observer(const MemoryPool* pool, char* msg, size_t len) {
#if OBSERVER_MODE
    if (!pool) {
        ERROR(ERR_PTR_NULL);
        return;
    }

    const Observer obs = pool->observer;

#if ENABLE_PRINT
    printf("内存池信息\n");
    printf("block总数: %zu\n", pool->block_num);
    printf("block大小: %zu\n", pool->block_size);
    printf("block实际跨度: %zu\n", pool->block_real_size);
    printf("当前已分配block数: %zu\n", obs.used_block_count);
    printf("历史最大已分配block数: %zu\n", obs.peak_used_block_count);
    printf("分配失败次数: %zu\n", obs.alloc_fail_count);
    printf("非法释放次数: %zu\n", obs.invalid_free_count);
    printf("重复释放次数: %zu\n", obs.double_free_count);
    printf("头部破坏次数: %zu\n", obs.header_corrupt_count);
    printf("尾部越界次数: %zu\n", obs.tail_overflow_count);
#else
    uint8_t index = 0;
    index += snprintf(msg + index, len, "%s内存池信息\n", msg);
    index += snprintf(msg + index, len, "block总数:%zu\n", pool->block_num);
    index +=
        snprintf(msg + index, len, "当前已分配:%zu\n", obs.used_block_count);
    index +=
        snprintf(msg + index, len, "历史峰值:%zu\n", obs.peak_used_block_count);
    index += snprintf(msg + index, len, "分配失败:%zu\n", obs.alloc_fail_count);
    index +=
        snprintf(msg + index, len, "重复释放:%zu\n", obs.double_free_count);
    index +=
        snprintf(msg + index, len, "头部破坏:%zu\n", obs.header_corrupt_count);
    index +=
        snprintf(msg + index, len, "尾部越界:%zu\n", obs.tail_overflow_count);
#endif
#else
    (void)pool;
    (void)msg;
    (void)len;
#endif
}
ERRCODE MemoryPool_Destory(MemoryPool** pool) {
    if (!pool || !(*pool)) {
        return ERR_OK;
    }
    if ((*pool)->observer.used_block_count != 0) {
        return ERROR(ERR_OPT, "当前内存池有未归还资源");
    }

    free(*pool);
    *pool = NULL;
    return ERR_OK;
}
ERRCODE MemoryPool_MemSet_Zero(MemoryPool* pool) {
    if (!pool) {
        return ERROR(ERR_PTR_NULL);
    }
    for (uint32_t i = 0; i < pool->block_num; i++) {
        uint8_t* block_base = (uint8_t*)pool->space + i * pool->block_real_size;

        uint8_t* user_area = block_base + sizeof(BlockListNode);
        memset(user_area, 0, pool->block_size);
    }
    return ERR_OK;
}
ERRCODE MemoryPool_Index(MemoryPool* pool, void* data, uint16_t* index) {
    if (!pool) {
        return ERR_PTR_NULL;
    }
    uint32_t start = (uint32_t)pool->space;
    uint32_t end = start + pool->block_num * pool->block_real_size;
    if ((uint32_t)data < start || (uint32_t)data >= end) {
        return ERR_ARGS;
    }
    *index = ((uint32_t)data - start) / pool->block_real_size;
    return ERR_OK;
}
void* MemoryPool_GetByIndex(MemoryPool* pool, uint16_t index) {
    if (!pool) {
        return NULL;
    }
    return pool->space + index * pool->block_real_size + sizeof(BlockListNode);
}
