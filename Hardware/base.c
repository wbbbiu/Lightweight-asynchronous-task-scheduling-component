#include "base.h"
volatile uint32_t g_sys_tick_ms=0;
volatile uint8_t sys_tick_flag=1;
volatile uint32_t G_TIM_Usage_Bitmap=0;
static uint32_t GPIO_Clocks[11]={RCC_AHB1Periph_GPIOA,RCC_AHB1Periph_GPIOB,RCC_AHB1Periph_GPIOC,RCC_AHB1Periph_GPIOD
    ,RCC_AHB1Periph_GPIOE,RCC_AHB1Periph_GPIOF,RCC_AHB1Periph_GPIOG,RCC_AHB1Periph_GPIOH,
    RCC_AHB1Periph_GPIOI,RCC_AHB1Periph_GPIOJ,RCC_AHB1Periph_GPIOK};
  static uint32_t tim_clocks1[]={
      RCC_APB1Periph_TIM2 ,RCC_APB1Periph_TIM3,RCC_APB1Periph_TIM4,RCC_APB1Periph_TIM5,
      RCC_APB1Periph_TIM6,RCC_APB1Periph_TIM7,
      RCC_APB1Periph_TIM12,RCC_APB1Periph_TIM13,RCC_APB1Periph_TIM14
};
static uint32_t tim_clocks2[]={
  RCC_APB2Periph_TIM1 ,RCC_APB2Periph_TIM8,RCC_APB2Periph_TIM9,RCC_APB2Periph_TIM10,RCC_APB2Periph_TIM11
};
static TIM_TypeDef *tims1[]={
 TIM2,TIM3,TIM4,TIM5,TIM6,TIM7,TIM12,TIM13,TIM14
};
static uint8_t tim_index1[]={
  2,3,4,5,6,7,12,13,14
 };
static TIM_TypeDef *tims2[]={
  TIM1,TIM8,TIM9,TIM10,TIM11
};
static uint8_t tim_index2[]={
  1,8,9,10,11
};
static inline uint16_t Get_Source(uint16_t pin){
     return __builtin_ctz(pin);
}
void GPIO_clock_init(GPIO_TypeDef *gpio){
     uint16_t index=((uint32_t)gpio-(uint32_t)GPIOA)/0X400;
     RCC_AHB1PeriphClockCmd(GPIO_Clocks[index],ENABLE);
}
TIM_Ret TIM_clock_init(TIM_TypeDef *tim){
    for(int i=0;i<(int)(sizeof(tims1)/sizeof(uint32_t));i++){
      if(tim==tims1[i]){
        RCC_APB1PeriphClockCmd(tim_clocks1[i],ENABLE);
        TIM_Ret ret={.id=tim_index1[i],.aph=1};
        return ret;
      }
    }
    for(int i=0;i<(int)(sizeof(tims2)/sizeof(uint32_t));i++){
      if(tim==tims2[i]){
        RCC_APB2PeriphClockCmd(tim_clocks2[i],ENABLE);
        TIM_Ret ret={.id=tim_index2[i],.aph=2};
        return ret;
      }
    }
    TIM_Ret ret={.id=0,.aph=1};
        return ret;
    return ret;
}
void NVIC_Configuration(uint8_t channel,uint8_t sub,uint8_t pre){

  NVIC_InitTypeDef INIT;
 // NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  INIT.NVIC_IRQChannel=channel;
  INIT.NVIC_IRQChannelCmd=ENABLE;
  INIT.NVIC_IRQChannelPreemptionPriority=sub;
  INIT.NVIC_IRQChannelSubPriority=pre;
  

  NVIC_Init(&INIT);
}
void Sys_Tick_Init(void){
  if(SysTick_Config(SystemCoreClock/1000)){
      sys_tick_flag=0;
  }
}
void SysTick_Handler(void){
    g_sys_tick_ms++;
}
void GPIO_OutInit(GPIO_TypeDef *gpio,uint16_t pin,uint16_t mode){
  GPIO_clock_init(gpio);
  GPIO_InitTypeDef INIT;
  INIT.GPIO_Mode=GPIO_Mode_OUT;
  INIT.GPIO_OType=GPIO_OType_PP;
  INIT.GPIO_Pin=pin;
  INIT.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(gpio,&INIT);

  if(!mode){
    GPIO_ResetBits(gpio,pin);
  }else{
    GPIO_SetBits(gpio,pin);
  }
}
void GPIO_AFOutInit(GPIO_TypeDef *gpio,uint16_t pin,uint8_t af){
  GPIO_clock_init(gpio);
  GPIO_InitTypeDef INIT;
  INIT.GPIO_Mode=GPIO_Mode_AF;
  INIT.GPIO_OType=GPIO_OType_PP;
  INIT.GPIO_Pin=pin;
  INIT.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(gpio,&INIT);
  GPIO_PinAFConfig(gpio,Get_Source(pin),af);
}

   
