#ifndef USE_H_
#define USE_H_
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
void user_init();
void print_str(int x,int y,char *p);
void print_int(int x,int y,int value);
void LCD_Clear_Line(u16 line);
#endif