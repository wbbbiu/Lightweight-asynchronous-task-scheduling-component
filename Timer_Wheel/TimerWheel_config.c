#include "TimerWheel_config.h"
volatile uint8_t TimeSlice_Trigger = 0;
typedef enum {
    TIM6_STATE_INIT,
    TIM6_STATE_WAITING,
    TIM6_STATE_EXPIRED
} TIM6_State;
volatile TIM6_State tim6_state;  // 定时状态
uint32_t next_trigger_ms;        // 下一个触发时间

void Timer_Wheel_TIM_Init() {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
    TIM_TimeBaseInitTypeDef INIT;
    TIM_TimeBaseStructInit(&INIT);
    INIT.TIM_Prescaler = 42000 - 1;
    INIT.TIM_ClockDivision = TIM_CKD_DIV1;
    INIT.TIM_CounterMode = TIM_CounterMode_Up;
    INIT.TIM_Period = 2 - 1;
    TIM_TimeBaseInit(TIM6, &INIT);

    NVIC_InitTypeDef NVIC_INIT;
    NVIC_INIT.NVIC_IRQChannel = TIM6_DAC_IRQn;
    NVIC_INIT.NVIC_IRQChannelPreemptionPriority = 15;
    NVIC_INIT.NVIC_IRQChannelSubPriority = 0;
    NVIC_INIT.NVIC_IRQChannelCmd = ENABLE;
    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);
    NVIC_Init(&NVIC_INIT);
}
void TIM6_SetInterval(uint32_t interval_ms) {
    TIM_Cmd(TIM6, DISABLE);
    TIM6->ARR = 2 * interval_ms - 1;
    TIM6->CNT = 0;
    TIM_ClearFlag(TIM6, TIM_FLAG_Update);
    TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM6, ENABLE);
}
void TimerClock_Update(TimerWheel* timer) {
    // 没初始化或者这个定时器触发需要处理
    if (tim6_state == TIM6_STATE_EXPIRED || tim6_state == TIM6_STATE_INIT) {
        // 减去之前的定时时间
        if (tim6_state == TIM6_STATE_INIT) {
            Timer_Wheel_TIM_Init();
        } else if (tim6_state == TIM6_STATE_EXPIRED) {
            timer->wheel_tick_remain_ms -= next_trigger_ms;
        }
        // 如果需要计时时间变成0，说明需要切换到下一个槽位
        if (timer->wheel_tick_remain_ms == 0) {
            timer->wheel_tick_remain_ms = timer->tick_ms;
            // 切换槽的前提是这个触发不是初始化导致的
            if (tim6_state != TIM6_STATE_INIT) {
                timer->current_slot++;
                timer->current_slot =
                    timer->current_slot & (timer->slot_num - 1);
            }
        }
        // 相比这个一次1ms的触发，我们根据当前需要计时时间，来调整定时器的周期
        if (timer->wheel_tick_remain_ms >= 10000) {
            next_trigger_ms = 10000;
        } else if (timer->wheel_tick_remain_ms >= 1000) {
            next_trigger_ms = 1000;
        } else if (timer->wheel_tick_remain_ms >= 100) {
            next_trigger_ms = 100;
        } else if (timer->wheel_tick_remain_ms >= 10) {
            next_trigger_ms = 10;
        } else {
            next_trigger_ms = timer->wheel_tick_remain_ms;
        }
        if (tim6_state == TIM6_STATE_INIT) {
            TIM6_SetInterval(next_trigger_ms);
        } else {
            TIM_Cmd(TIM6, DISABLE);
            TIM6_SetInterval(next_trigger_ms);
        }
        tim6_state = TIM6_STATE_WAITING;
    }
}
void TIM6_DAC_IRQHandler(void) {
    if (TIM_GetITStatus(TIM6, TIM_IT_Update) == SET) {
        tim6_state = TIM6_STATE_EXPIRED;
        //   printf("TIM6_DAC_IRQHandler\r\n");
        TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
    }
}
void TimeSlice_Add(TimerWheel* timer, uint32_t Add_ms) {
    if (!timer || timer->is_shut || Add_ms <= 0 || timer->tick_ms <= 0) {
        return;
    }
    timer->wheel_tick_remain_ms += Add_ms;
    if (timer->wheel_tick_remain_ms >= timer->tick_ms) {
        timer->current_slot++;
        timer->current_slot = timer->current_slot & (timer->slot_num - 1);
        timer->wheel_tick_remain_ms = 0;
    }
}