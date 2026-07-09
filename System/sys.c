#include "sys.h"

// 使用 AC6 兼容的内联汇编或 CMSIS 内核接口

// 执行 WFI 指令，进入低功耗待机
void WFI_SET(void)
{
    __asm__ __volatile__("wfi");
}

// 关闭所有中断 (除了 Fault 和 NMI)
void INTX_DISABLE(void)
{
    __asm__ __volatile__("cpsid i");
}

// 开启所有中断
void INTX_ENABLE(void)
{
    __asm__ __volatile__("cpsie i");
}

// 设置主堆栈指针
// addr: 栈顶地址
void MSR_MSP(u32 addr) 
{
    __set_MSP(addr);
}