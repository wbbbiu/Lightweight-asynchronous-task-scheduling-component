#ifndef _SPI_BASE_H_
#define _SPI_BASE_H_
#include "stm32f4xx.h"
extern volatile uint32_t g_sys_tick_ms;
extern volatile uint8_t sys_tick_flag;
extern volatile uint32_t G_TIM_Usage_Bitmap;
typedef struct TIM_Ret {
    uint8_t id : 6;
    uint8_t aph : 2;
} TIM_Ret;
typedef struct Gpio_Base {
    GPIO_TypeDef* port;
    uint16_t pin;
} Gpio_Base;
void GPIO_clock_init(GPIO_TypeDef* gpio);
TIM_Ret TIM_clock_init(TIM_TypeDef* tim);
void Sys_Tick_Init(void);
static inline uint32_t get_sys_tick() { return g_sys_tick_ms; }
void NVIC_Configuration(uint8_t channel, uint8_t sub, uint8_t pre);
void GPIO_OutInit(GPIO_TypeDef* gpio, uint16_t pin, uint16_t mode);
void GPIO_AFOutInit(GPIO_TypeDef* gpio, uint16_t pin, uint8_t af);
#endif