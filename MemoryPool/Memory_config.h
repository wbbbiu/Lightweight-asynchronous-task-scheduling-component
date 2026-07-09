#ifndef MEMPOOL_DEFAULT_CONFIG_H
#define MEMPOOL_DEFAULT_CONFIG_H
#include <stdint.h>
// 多任务保护
#ifndef MEMPOOL_MULTI_TASK_SAFE
#define MEMPOOL_MULTI_TASK_SAFE 0
#endif
// canary模式检查向后越界为0关闭
#ifndef MEMPOOL_CANARY
#define MEMPOOL_CANARY 1
#endif
#if MEMPOOL_CANARY
#define CANARY_SIZE 4U
#else
#define CANARY_SIZE 0U
#endif
// magic模式检查向前越界和这个野指针释放为0关闭
#ifndef MEMPOOL_MAGIC
#define MEMPOOL_MAGIC 1
#endif
// free模式检查double free为0关闭
#ifndef MEMPOOL_CHECK_FREE
#define MEMPOOL_CHECK_FREE 1
#endif
// 野指针检查为0关闭
#ifndef MEMPOOL_WILD_FREE_CHECK
#define MEMPOOL_WILD_FREE_CHECK 1
#endif

#ifndef OBSERVER_MODE
#define OBSERVER_MODE 1
#endif

#define MAGIC 0xDEADC0DE
#define CANARY 0xCAFEBABE
void pool_lock(void);
void pool_unlock(void);
#if MEMPOOL_MULTI_TASK_SAFE
#define MEMPOOL_LOCK() pool_lock()
#define MEMPOOL_UNLOCK() pool_unlock()
#else
#define MEMPOOL_LOCK() \
    do {               \
    } while (0)
#define MEMPOOL_UNLOCK() \
    do {                 \
    } while (0)
#endif

#endif