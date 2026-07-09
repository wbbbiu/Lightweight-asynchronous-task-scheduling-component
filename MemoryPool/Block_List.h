#ifndef BLOCK_LIST_H
#define BLOCK_LIST_H

#include <stddef.h>
#include <stdint.h>
#include "Error.h"
#include "Memory_config.h"

typedef struct BlockListNode BlockListNode;
struct BlockListNode {
    // 用于这个块的重复释放检查，1 表示已释放，0 表示未释放
#if MEMPOOL_CHECK_FREE
    uint32_t state;
#endif
    BlockListNode* next;
    // 用于这个块的向前越界检查，1 表示已越界，0 表示未越界
#if MEMPOOL_MAGIC
    uint32_t magic;
#endif
};

static inline ERRCODE BlockList_push(BlockListNode** list,
                                     BlockListNode* node) {
    if (!list || !node) {
        return ERROR(ERR_PTR_NULL);
    }
    node->next = *list;
    *list = node;
    return ERR_OK;
}

static inline BlockListNode* BlockList_pop(BlockListNode** list) {
    if (!list || !*list) {
        return NULL;
    }
    BlockListNode* node = *list;
    *list = node->next;
    node->next = NULL;
    return node;
}
#endif