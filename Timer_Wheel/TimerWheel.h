#ifndef TIMER_WHEEL_H
#define TIMER_WHEEL_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "Error.h"
#include "TimerWheel_Struct.h"
#include "TimerWheel_config.h"

#ifndef MAX_TIMERS
#define MAX_TIMERS 129
#endif
#define FOREVER 0xffff
#define TIMER_INVALID_ID 0xffffffffU
/*
 * @brief 创建软件定时器
 * @param precision_ms 软件定时器的时间精度，单位：ms
 * @param slot_num     定时器槽的数目，必须是2的n次方，向下取比如65结果是64
 * @param clock_mode   两种模式模式：
 *  TIMER_WHEEL_MODE_TIME 定时器模式使用系定时器tim6实现
 *  TIMER_WHEEL_MODE_TICK  tick模式，
 * 用户自己使用这个void TimerWheel_Adance(TimerWheel* timer, uint32_t
 * Add_ms)来更新定时器时间
 * @param max_timer_num 最大定时器数量
 * @return TimerWheel* 定时轮指针
 */
TimerWheel* Create_Timer_Wheel(uint16_t precision_ms, uint16_t slot_num,
                               uint32_t clock_mode, uint16_t max_timer_num);
/*
 * @brief 提交任务到定时器
 * @param timer 定时器指针
 * @param task
 * 任务指针，任务需要参数，间隔时间，任务重复次数，无限重复使用FORERVE
 * @param arg  任务参数
 * @param interval_ms 间隔时间，单位：ms
 * @param repeat 任务重复次数，无限重复使用FORERVE
 * @param timer_id 任务id指针，提交成功后会将任务id赋值给timer_id
 * @return ERRCODE 错误码
 */
ERRCODE Timer_Wheel_Submit_Task(TimerWheel* timer, void* (*task)(void*),
                                void* arg, uint32_t interval_ms, int repeat,
                                uint32_t* timer_id);
/*
 * @brief 给出这个任务id删除这个任务,删除成功返回0，失败返回传入id
 * @param timer 定时器指针
 * @param id 任务id
 * @return ERRCODE 错误码
 */
ERRCODE Timer_Wheel_Cancel_Task(TimerWheel* timer, uint32_t id);
/*
 * @brief 时间轮到达触发定时器
 * @param timer 定时器指针
 */
ERRCODE Timer_Wheel_Loop(TimerWheel* timer);
/*
 * @brief 展示这个软件定时器信息
 * @param timer 定时器指针
 */
void Timer_Wheel_Show(const TimerWheel* timer);
/*
 * @brief 软件定时器的销毁
 * @param timer 定时器指针
 */
void Timer_Wheel_Destory(TimerWheel** timer);
/*
 * @brief 暂停软件定时器
 * @param timer 定时器指针
 */
void Timer_Wheel_Stop(TimerWheel* timer);
/*
 * @brief 重新提交任务到定时器
 * @param node 任务节点指针
 * @param timer 定时器指针
 * @param cur_slot 当前槽
 */
ERRCODE Timer_Node_Resubmit(TimerNode* node, TimerWheel* timer,
                            uint16_t cur_slot);
#endif