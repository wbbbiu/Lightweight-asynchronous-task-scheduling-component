#ifndef TEST_USART_H
#define TEST_USART_H
#include "user_usart.h"
#define TEST_USART USART2
#define TEST_USART_RX_PIN GPIO_Pin_3
#define TEST_USART_TX_PIN GPIO_Pin_2
#define TEST_USART_RX_PORT GPIOA
#define TEST_USART_TX_PORT GPIOA
#define TEST_USART_BAUD 115200
void Test_USART_Init(void);
#endif