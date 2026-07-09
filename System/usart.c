#include "usart.h"

#include <rt_sys.h>
#include <stdio.h>

#include "sys.h"

#if SYSTEM_SUPPORT_OS
#include "includes.h"
#endif

//////////////////////////////////////////////////////////////////
// 加入以下代码,支持 printf 函数,而不需要选择 use MicroLIB
#if 1

// 屏蔽半主机模式 (Arm Compiler 6 专用语法)
__asm(".global __use_no_semihosting\n\t");

// 定义标准库需要的支持函数

void _sys_exit(int x) { (void)x; }
void _ttywrch(int ch) { (void)ch; }

FILEHANDLE _sys_open(const char* name, int openmode) { return 0; }
int _sys_close(FILEHANDLE fh) { return 0; }
int _sys_write(FILEHANDLE fh, const unsigned char* buf, unsigned int len,
               int mode) {
    return 0;
}
int _sys_read(FILEHANDLE fh, unsigned char* buf, unsigned int len, int mode) {
    return 0;
}
int _sys_istty(FILEHANDLE fh) { return 0; }
int _sys_seek(FILEHANDLE fh, long pos) { return 0; }
long _sys_flen(FILEHANDLE fh) { return 0; }
// __stdout 已经在 stdio.h 中处理，此处无需重复定义 struct __FILE

// // 重定义 fputc 函数
// int fputc(int ch, FILE *f)
// {
//     while((USART1->SR & 0X40) == 0); // 等待发送完成
//     USART1->DR = (uint8_t)ch;
//     return ch;
// }

#endif

#if EN_USART1_RX
// 接收缓冲区
u8 USART_RX_BUF[USART_REC_LEN];
// 接收状态标记
u16 USART_RX_STA = 0;

// 初始化 IO 串口1
void uart_init(u32 bound) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1, ENABLE);

#if EN_USART1_RX
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif
}

void USART1_IRQHandler(void) {
    u8 Res;
#if SYSTEM_SUPPORT_OS
    OSIntEnter();
#endif
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        Res = USART_ReceiveData(USART1);

        if ((USART_RX_STA & 0x8000) == 0) {
            if (USART_RX_STA & 0x4000) {
                if (Res != 0x0a)
                    USART_RX_STA = 0;
                else
                    USART_RX_STA |= 0x8000;
            } else {
                if (Res == 0x0d)
                    USART_RX_STA |= 0x4000;
                else {
                    USART_RX_BUF[USART_RX_STA & 0X3FFF] = Res;
                    USART_RX_STA++;
                    if (USART_RX_STA > (USART_REC_LEN - 1)) USART_RX_STA = 0;
                }
            }
        }
    }
#if SYSTEM_SUPPORT_OS
    OSIntExit();
#endif
}
#endif