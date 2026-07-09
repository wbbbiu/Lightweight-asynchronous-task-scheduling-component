#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Block_List.h"
#include "MemoryPool.h"
#include "Test_Usart.h"
#include "core_cm4.h"
#include "stm32f4xx.h"
/*****************************************************************************
 * 文件名: timer_wheel_test.c
 * 描述: 软件定时器(TimerWheel) 在 STM32F407 上的测试代码
 * 硬件: STM32F407, 使用 printf 输出调试信息, get_sys_tick() 获取系统毫秒
 * 说明: 请将此文件与你的 TimerWheel.c/.h 一起加入工程编译
 *****************************************************************************/

#include <stdint.h>
#include <stdio.h>

#include "TimerWheel.h"
#include "TimerWheel_config.h"

typedef struct {
    const char* name;
} TestTaskArg;

static TimerWheel* g_timer = NULL;

static uint32_t g_start_ms = 0;
static uint32_t g_last_tick_ms = 0;

static uint32_t g_forever_id = TIMER_INVALID_ID;
static uint8_t g_forever_cancelled = 0;

static uint32_t Test_ElapsedMs(void) { return get_sys_tick() - g_start_ms; }

// 打印时间前缀
static void Print_TimePrefix(void) {
    printf("[%lu ms] ", (unsigned long)Test_ElapsedMs());
}

static void* Test_Callback(void* arg) {
    TestTaskArg* task_arg = (TestTaskArg*)arg;

    Print_TimePrefix();

    if (task_arg && task_arg->name) {
        printf("%s run\r\n", task_arg->name);
    } else {
        printf("unknown task run\r\n");
    }

    return NULL;
}

void TimerWheel_Test_Start(void) {
    g_forever_cancelled = 0;
    g_forever_id = TIMER_INVALID_ID;
    // 创建软件定时器(TimerWheel) 实例
    g_timer = Create_Timer_Wheel(500,  // tick_ms = 500ms
                                 8,    // slot_num = 8，一圈 4000ms
                                 TIMER_WHEEL_MODE_TIME,
                                 16  // 最大任务数量
    );

    if (!g_timer) {
        printf("TimerWheel create failed\r\n");
        return;
    }
    // 立即执行的任务
    static TestTaskArg task_immediate = {.name = "immediate task"};
    // 一次性延迟1000ms
    static TestTaskArg task_oneshot_1000 = {.name = "oneshot 1000ms"};
    // 重复3次，每1500ms执行一次
    static TestTaskArg task_repeat_1500 = {.name =
                                               "repeat 3 times every 1500ms"};

    static TestTaskArg task_cross_round_5000 = {
        .name = "cross-round oneshot 5000ms"};
    // 3000ms 后取消的任务
    static TestTaskArg task_cancel = {.name = "cancelled task should NOT run"};
    // 永久执行的任务
    static TestTaskArg task_forever = {.name = "forever every 2000ms"};

    uint32_t id = TIMER_INVALID_ID;
    uint32_t cancel_id = TIMER_INVALID_ID;

    printf("========== TimerWheel Test Start ==========\r\n");
    printf("tick_ms = 500ms, slot_num = 8, wheel_cycle = 4000ms\r\n");

    /*
     * 1. 立即执行任务
     * repeat = 0，应该马上执行，不进入时间轮，id 为 TIMER_INVALID_ID。
     */
    ERRCODE ret = ERR_OK;
    ret = Timer_Wheel_Submit_Task(g_timer, Test_Callback, &task_immediate, 0, 0,
                                  &id);
    if (ret != ERR_OK) {
        printf("submit immediate task failed\r\n");
        ret = ERR_OK;
    }
    printf("immediate task id = 0x%08lx\r\n", (unsigned long)id);

    /*
     * 2. 一次性延迟任务：1000ms 后执行一次
     */

    ret = Timer_Wheel_Submit_Task(g_timer, Test_Callback, &task_oneshot_1000,
                                  1000, 1, &id);
    if (ret != ERR_OK) {
        printf("submit oneshot 1000ms failed\r\n");
        ret = ERR_OK;
    }
    printf("oneshot 1000ms id = 0x%08lx\r\n", (unsigned long)id);

    /*
     * 3. 重复 3 次：每 1500ms 执行一次
     */
    ret = Timer_Wheel_Submit_Task(g_timer, Test_Callback, &task_repeat_1500,
                                  1500, 3, &id);
    if (ret != ERR_OK) {
        printf("submit repeat 1500ms failed\r\n");
        ret = ERR_OK;
    }
    printf("repeat 1500ms id = 0x%08lx\r\n", (unsigned long)id);

    /*
     * 4. 跨 round 任务：5000ms 后执行一次
     * 因为一圈是 4000ms，所以这个任务会测试 round 机制。
     */
    ret = Timer_Wheel_Submit_Task(g_timer, Test_Callback,
                                  &task_cross_round_5000, 5000, 1, &id);
    if (ret != ERR_OK) {
        printf("submit cross-round 5000ms failed\r\n");
        ret = ERR_OK;
    }
    printf("cross-round 5000ms id = 0x%08lx\r\n", (unsigned long)id);

    /*
     * 5. cancel 测试：3000ms 任务，马上取消，预期不应该打印回调。
     */
    ret = Timer_Wheel_Submit_Task(g_timer, Test_Callback, &task_cancel, 3000, 1,
                                  &cancel_id);
    if (ret != ERR_OK) {
        printf("submit cancel task failed\r\n");
        ret = ERR_OK;
    }
    printf("cancel task id = 0x%08lx\r\n", (unsigned long)cancel_id);

    ret = Timer_Wheel_Cancel_Task(g_timer, cancel_id);
    if (ret != ERR_OK) {
        printf("cancel cancel task failed\r\n");
        ret = ERR_OK;
    }
    printf("cancel task submitted then cancelled\r\n");

    /*
     * 6. FOREVER 测试：每 2000ms 执行，7000ms 后取消。
     */
    ret = Timer_Wheel_Submit_Task(g_timer, Test_Callback, &task_forever, 2000,
                                  FOREVER, &g_forever_id);
    if (ret != ERR_OK) {
        printf("submit forever 2000ms failed\r\n");
        ret = ERR_OK;
    }
    printf("forever 2000ms id = 0x%08lx\r\n", (unsigned long)g_forever_id);
}

void TimerWheel_Test_Loop(void) {
    Timer_Wheel_Loop(g_timer);
    if (!g_timer) {
        return;
    }

    /*
     * 7000ms 后取消 FOREVER 任务。
     * 用 >=，不要用 ==，避免主循环错过精确时间点。
     */
    if (!g_forever_cancelled && Test_ElapsedMs() >= 7000) {
        g_forever_cancelled = 1;

        Print_TimePrefix();
        printf("cancel forever task\r\n");

        Timer_Wheel_Cancel_Task(g_timer, g_forever_id);
    }

    /*
     * 12000ms 后结束测试。
     */
    if (Test_ElapsedMs() >= 12000) {
        printf("========== TimerWheel Test Finish ==========\r\n");

        Timer_Wheel_Show(g_timer);

        Timer_Wheel_Destory(&g_timer);
    }
}

/*=======================================================================*
 *  第3部分: 主函数 — 测试流程
 *=======================================================================*/
int main(void) {
    Sys_Tick_Init();
    Test_USART_Init();
    TimerWheel_Test_Start();
    g_start_ms = get_sys_tick();
    while (1) {
        TimerWheel_Test_Loop();
    }
}