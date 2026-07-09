#ifndef TIMER_PORT_H
#define TIMER_PORT_H
#include "Error.h"
#include "TimerWheel_Struct.h"
#include "stm32f4xx.h"
#define TIMER_WHEEL_MODE_TIME 1
#define TIMER_WHEEL_MODE_TICK 2
void TimerClock_Update(TimerWheel* timer);
void TimerWheel_Adance(TimerWheel* timer, uint32_t Add_ms);
void Timer_Wheel_TIM_Init();
#endif