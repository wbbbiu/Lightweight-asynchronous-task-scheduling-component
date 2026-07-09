#ifndef TIMER_WHEEL_STRUCT_H
#define TIMER_WHEEL_STRUCT_H
#include <stdint.h>

#include "MemoryPool.h"
#include "TimerNode.h"
typedef struct TimerWheelOpt TimerWheelOpt;
typedef struct TimerWheel TimerWheel;

struct TimerWheelOpt {
    ERRCODE (*Submit_Task)(TimerWheel* timer, void* (*task)(void*), void* arg,
                           uint32_t interval_ms, int repeat,
                           uint32_t* timer_id);
    ERRCODE (*Cancel_Task)(TimerWheel* timer, uint32_t id);
    void (*Show)(const TimerWheel* timer);
    void (*Destory)(TimerWheel** timer);
    ERRCODE (*Loop)(TimerWheel* timer);
    void (*Stop)(TimerWheel* timer);
};
struct TimerWheel {
    // 槽对应的位移量
    uint8_t slot_bit;
    // 是否关闭
    uint16_t is_shut;
    // 槽数
    uint16_t slot_num;
    // 当前槽位置
    uint16_t current_slot;
    uint16_t last_slot;
    // 当前激活任务数
    uint16_t active_count;
    // 最大定时器数量
    uint16_t max_timer_num;

    // 时间片ms，也是基本精度
    uint32_t tick_ms;
    // 和下一个时间片触发的剩余时间ms
    uint32_t wheel_tick_remain_ms;  // 剩余时间ms
    // 已经触发的个数
    uint32_t completed_count;
    uint32_t submit_count;
    uint32_t cancel_count;

    uint32_t clock_mode;
    TimerWheelOpt opt;
    MemoryPool* timer_pool;
    TimerNode* slot[];
};
#endif