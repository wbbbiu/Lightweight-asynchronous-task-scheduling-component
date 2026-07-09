#include "Test_Usart.h"

#include <stdio.h>

Usart_Device Test_USART_Device;
int fputc(int ch, FILE* f) {
    while (USART_GetFlagStatus(Test_USART_Device.usart, USART_FLAG_TXE) ==
           RESET);
    USART_SendData(Test_USART_Device.usart, (uint8_t)ch);
    return ch;
}
void Test_USART_Init(void) {
    Test_USART_Device =
        usart_create(TEST_USART_BAUD, TEST_USART, TEST_USART_RX_PORT,
                     TEST_USART_TX_PORT, TEST_USART_RX_PIN, TEST_USART_TX_PIN);
    user_usart_init(&Test_USART_Device);
}