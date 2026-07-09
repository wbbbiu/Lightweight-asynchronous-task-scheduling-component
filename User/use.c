#include "use.h"
#include "base.h"
void user_init(){
    delay_init(168);      
    uart_init(115200); 
    LCD_Init();    

}
void print_str(int x,int y,char *p){
    POINT_COLOR = BLUE;   
    LCD_ShowString(x*16, y*16, 210, 16, 16, p);
}
void print_int(int x,int y,int value){
    POINT_COLOR = BLUE;   
    LCD_ShowNum(x*16==0?1:x*16, y*16==0?1:y*16, value, 6, 16);
}
// 清空第 line 行，用背景色清空
void LCD_Clear_Line(u16 line)
{
    u16 pixel_y = line * 16;        // 字符行 → 像素起始行
    LCD_Fill(0, pixel_y, 239, pixel_y + 15, WHITE);
}