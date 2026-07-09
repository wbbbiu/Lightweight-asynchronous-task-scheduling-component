#ifndef TIMER_NODE_H
#define TIMER_NODE_H
#include <stdint.h>
#include <stdlib.h>

#include "Error.h"

typedef struct TimerNode TimerNode;
struct TimerNode {
    // 代表这个任务重复次数，如果是uint16的最大值那么就是永远重复
    uint16_t repeat;
    // 时间间隔ms
    uint16_t interval_ms;
    // 在当前槽中第几轮
    uint32_t round;
    void* (*callback)(void* arg);
    void* arg;
    // 下一个节点
    TimerNode* next;
    // 上一个节点
    TimerNode* pre;
    // 在内存中的下标
    uint16_t memory_index;
    // 此内存存放第几次任务
    uint16_t version;
};
// 插入节点到链表头
static inline ERRCODE TimerNode_Push(TimerNode** head, TimerNode* node) {
    if (!head || !node) {
        return ERROR(ERR_ARGS, "节点不能为空");
    }
    node->next = *head;
    node->pre = NULL;
    if (*head) {
        (*head)->pre = node;
    }
    *head = node;
    return ERR_OK;
}
static inline ERRCODE TimerNode_Del(TimerNode** head, TimerNode* node) {
    if (!head || !(*head) || !node) {
        return ERROR(ERR_ARGS, "节点不能为空");
    }
    if (node == *head) {
        *head = node->next;
        if (*head) {
            (*head)->pre = NULL;
        }
    } else {
        if (node->pre) {
            node->pre->next = node->next;
        }
        if (node->next) {
            node->next->pre = node->pre;
        }
    }
    node->next = NULL;
    node->pre = NULL;
    return ERR_OK;
}
#endif