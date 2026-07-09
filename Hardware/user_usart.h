#ifndef USER_USART_H
#define USER_USART_H
#include "base.h"
#include "stm32f4xx.h"

#define OCCUPIED_USED_USART 0b00000001

#define RECV_NVIC 1
extern char buf[64];
typedef struct Usart_Device {
    GPIO_TypeDef* RX_port;
    GPIO_TypeDef* TX_port;
    uint16_t RX_pin;
    uint16_t TX_pin;
    uint32_t baud : 31;
    uint32_t flag : 1;
    USART_TypeDef* usart;
} Usart_Device;
Usart_Device usart_create(int baud, USART_TypeDef* usart, GPIO_TypeDef* rx_port,
                          GPIO_TypeDef* tx_port, uint16_t rx_pin,
                          uint16_t tx_pin);
void user_usart_init(Usart_Device* device);
void usart_send_char(Usart_Device* device, uint8_t ch);
void usart_send_int(Usart_Device* device, int val);
void usart_send_str(Usart_Device* device, char* str);
void usart_nvic_register(Usart_Device* device,
                         void (*func)(uint8_t data, void* context),
                         void* context);


#endif