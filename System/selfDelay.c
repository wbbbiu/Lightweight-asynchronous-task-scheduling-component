#include "selfDelay.h"

void Delay_ms(uint16_t time){
    uint32_t start_time=get_sys_tick();
   while(get_sys_tick()-start_time>time){
         
   }
}
void Delay_s(uint16_t time){
    while (time--)
    {
        Delay_ms(1000);
    }
}