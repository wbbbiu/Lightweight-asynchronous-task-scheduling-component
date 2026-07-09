#include "user_usart.h"

#include <stdio.h>

static inline uint16_t Get_Source_Pin(uint16_t pin) {
    uint16_t ret = __builtin_ctz(pin);
    return ret;
}
static void (*usart_callback[8])(uint8_t data, void* context) = {0};
static void* contexts[8] = {0};
static inline uint8_t Get_AF_val(USART_TypeDef* usart) {
    if (usart == USART1 || usart == USART2 || usart == USART3) {
        return 0X07;
    } else if (usart == UART4 || usart == UART5 || usart == USART6) {
        return 0x08;
    }
    return 0X07;
}
static inline void usart_clock_init(USART_TypeDef* usart) {
    if (usart == USART1)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    else if (usart == USART6)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
    else if (usart == USART2)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    else if (usart == USART3)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
}
static inline uint8_t Get_irq(USART_TypeDef* usart) {
    if (usart == USART1)
        return USART1_IRQn;
    else if (usart == USART2)
        return USART2_IRQn;
    else if (usart == USART3)
        return USART3_IRQn;
    else if (usart == UART4)
        return UART4_IRQn;
    else if (usart == UART5)
        return UART5_IRQn;
    else if (usart == USART6)
        return USART6_IRQn;
#if defined(STM32F427_437xx)
    else if (usart == UART7)
        return UART7_IRQn;
    else if (usart == UART8)
        return UART8_IRQn;
#endif
    else
        return USART1_IRQn;
}
Usart_Device usart_create(int baud, USART_TypeDef* usart, GPIO_TypeDef* rx_port,
                          GPIO_TypeDef* tx_port, uint16_t rx_pin,
                          uint16_t tx_pin) {
    if (baud < 0) {
        baud = -baud;
    }
    Usart_Device device = {.baud = baud,
                           .flag = 0,
                           .RX_pin = rx_pin,
                           .RX_port = rx_port,
                           .TX_pin = tx_pin,
                           .TX_port = tx_port,
                           .usart = usart};
    return device;
}
void user_usart_init(Usart_Device* device) {
    GPIO_clock_init(device->RX_port);
    GPIO_clock_init(device->TX_port);
    usart_clock_init(device->usart);

    GPIO_PinAFConfig(device->RX_port, Get_Source_Pin(device->RX_pin),
                     Get_AF_val(device->usart));
    GPIO_PinAFConfig(device->TX_port, Get_Source_Pin(device->TX_pin),
                     Get_AF_val(device->usart));

    GPIO_InitTypeDef INIT;
    INIT.GPIO_Mode = GPIO_Mode_AF;
    INIT.GPIO_OType = GPIO_OType_PP;
    INIT.GPIO_Pin = device->RX_pin;
    INIT.GPIO_PuPd = GPIO_PuPd_UP;
    INIT.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(device->RX_port, &INIT);
    INIT.GPIO_Pin = device->TX_pin;
    GPIO_Init(device->TX_port, &INIT);

    USART_InitTypeDef INIT_US;
    INIT_US.USART_BaudRate = device->baud;
    INIT_US.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    INIT_US.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    INIT_US.USART_Parity = USART_Parity_No;
    INIT_US.USART_StopBits = USART_StopBits_1;
    INIT_US.USART_WordLength = USART_WordLength_8b;

    USART_Init(device->usart, &INIT_US);
    USART_Cmd(device->usart, ENABLE);
#if (RECV_NVIC)
    NVIC_Configuration(Get_irq(device->usart), 1, 1);
    USART_ITConfig(device->usart, USART_IT_RXNE, ENABLE);
#endif
    device->flag = 1;
}

void usart_send_char(Usart_Device* device, uint8_t ch) {
    if (device->flag == 0) return;
    while (USART_GetFlagStatus(device->usart, USART_SR_TXE) == RESET);
    USART_SendData(device->usart, ch);
}
void usart_send_int(Usart_Device* device, int val) {
    if (device->flag == 0) return;
    if (val < 0) {
        usart_send_char(device, '-');
        val = -val;
    }
    if (val == 0) {
        usart_send_char(device, '0');
    }
    char buf[32];
    uint8_t len = 0;
    while (val) {
        buf[len++] = val % 10 + '0';
        val = val / 10;
    }
    while (len--) {
        usart_send_char(device, buf[len]);
    }
}
void usart_send_str(Usart_Device* device, char* str) {
    if (device->flag == 0) return;
    while (*str) {
        usart_send_char(device, *str++);
    }
}
// int fputc(int ch, FILE *f){
//      while(USART_GetFlagStatus(USART1,USART_SR_TXE)==RESET);
//      USART_SendData(USART1,(uint8_t)ch);
//      return ch;
// }
#if (RECV_NVIC)
void usart_nvic_register(Usart_Device* device,
                         void (*func)(uint8_t data, void* context),
                         void* context) {
    // print_str(0,4,"rigister:");
    if (device->usart == USART1) {
        usart_callback[0] = func;
        contexts[0] = context;
    } else if (device->usart == USART2) {
        // print_str(0,5,"rigister sucee:");
        usart_callback[1] = func;
        contexts[1] = context;
    } else if (device->usart == USART3) {
        usart_callback[2] = func;
        contexts[2] = context;
    } else if (device->usart == UART4) {
        usart_callback[3] = func;
        contexts[3] = context;
    } else if (device->usart == UART5) {
        usart_callback[4] = func;
        contexts[4] = context;
    } else if (device->usart == USART6) {
        usart_callback[5] = func;
        contexts[5] = context;
    } else if (device->usart == UART7) {
        usart_callback[6] = func;
        contexts[6] = context;
    } else if (device->usart == UART8) {
        usart_callback[7] = func;
        contexts[7] = context;
    }
}
#if (!(OCCUPIED_USED_USART & 0X01))
void USART1_IRQHandler(void) {
    if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET) {
        uint16_t temp = USART1->SR;
        temp = USART1->DR;
        (void)temp;
    }
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET) {
        uint8_t data = (uint8_t)USART_ReceiveData(USART1);
        if (usart_callback[0]) {
            usart_callback[0](data, contexts[0]);
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
#endif
#if (!(OCCUPIED_USED_USART & 0X02))
void USART2_IRQHandler(void) {
    // print_str(0,3,"s2");
    if (USART_GetFlagStatus(USART2, USART_FLAG_ORE) != RESET) {
        uint16_t temp = USART2->SR;
        temp = USART2->DR;
        (void)temp;
    }
    if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET) {
        uint8_t data = (uint8_t)USART_ReceiveData(USART2);
        if (usart_callback[1]) {
            usart_callback[1](data, contexts[1]);
        }
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}
#endif
#if (!(OCCUPIED_USED_USART & 0X04))
void USART3_IRQHandler(void) {
    if (USART_GetFlagStatus(USART3, USART_FLAG_ORE) != RESET) {
        uint16_t temp = USART3->SR;
        temp = USART3->DR;
        (void)temp;
    }
    if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET) {
        uint8_t data = (uint8_t)USART_ReceiveData(USART3);
        if (usart_callback[2]) {
            usart_callback[2](data, contexts[2]);
        }
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}
#endif
#if (!(OCCUPIED_USED_USART & 0X08))
void UART4_IRQHandler(void) {
    if (USART_GetFlagStatus(UART4, USART_FLAG_ORE) != RESET) {
        uint16_t temp = UART4->SR;
        temp = UART4->DR;
        (void)temp;
    }
    if (USART_GetITStatus(UART4, USART_IT_RXNE) == SET) {
        uint8_t data = (uint8_t)USART_ReceiveData(UART4);
        if (usart_callback[3]) {
            usart_callback[3](data, contexts[3]);
        }
        USART_ClearITPendingBit(UART4, USART_IT_RXNE);
    }
}
#endif
#if (!(OCCUPIED_USED_USART & 0X10))
void UART5_IRQHandler(void) {
    if (USART_GetFlagStatus(UART5, USART_FLAG_ORE) != RESET) {
        uint16_t temp = UART5->SR;
        temp = UART5->DR;
        (void)temp;
    }
    if (USART_GetITStatus(UART5, USART_IT_RXNE) == SET) {
        uint8_t data = (uint8_t)USART_ReceiveData(UART5);
        if (usart_callback[4]) {
            usart_callback[4](data, contexts[4]);
        }
        USART_ClearITPendingBit(UART5, USART_IT_RXNE);
    }
}
#endif
#if (!(OCCUPIED_USED_USART & 0X20))
void USART6_IRQHandler(void) {
    if (USART_GetFlagStatus(USART6, USART_FLAG_ORE) != RESET) {
        uint16_t temp = USART6->SR;
        temp = USART6->DR;
        (void)temp;
    }
    if (USART_GetITStatus(USART6, USART_IT_RXNE) == SET) {
        uint8_t data = (uint8_t)USART_ReceiveData(USART6);
        if (usart_callback[5]) {
            usart_callback[5](data, contexts[5]);
        }
        USART_ClearITPendingBit(USART6, USART_IT_RXNE);
    }
}
#endif
#if (!(OCCUPIED_USED_USART & 0X40))
void UART7_IRQHandler(void) {
    if (USART_GetFlagStatus(UART7, USART_FLAG_ORE) != RESET) {
        uint16_t temp = UART7->SR;
        temp = UART7->DR;
        (void)temp;
    }
    if (USART_GetITStatus(UART7, USART_IT_RXNE) == SET) {
        uint8_t data = (uint8_t)USART_ReceiveData(UART7);
        if (usart_callback[6]) {
            usart_callback[6](data, contexts[6]);
        }
        USART_ClearITPendingBit(UART7, USART_IT_RXNE);
    }
}
#endif
#if (!(OCCUPIED_USED_USART & 0X80))
void UART8_IRQHandler(void) {
    if (USART_GetFlagStatus(UART8, USART_FLAG_ORE) != RESET) {
        uint16_t temp = UART8->SR;
        temp = UART8->DR;
        (void)temp;
    }
    if (USART_GetITStatus(UART8, USART_IT_RXNE) == SET) {
        uint8_t data = (uint8_t)USART_ReceiveData(UART8);
        if (usart_callback[7]) {
            usart_callback[7](data, contexts[7]);
        }
        USART_ClearITPendingBit(UART8, USART_IT_RXNE);
    }
}
#endif
#endif
