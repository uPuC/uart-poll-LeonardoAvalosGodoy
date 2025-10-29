#ifndef UART_H
#define UART_H

#include <stdint.h>


void UART_Ini(uint8_t com, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop);


void UART_putchar(uint8_t com, char data);
void UART_puts(uint8_t com, char *str);


uint8_t UART_available(uint8_t com);
char    UART_getchar(uint8_t com);
void    UART_gets(uint8_t com, char *str);


void UART_clrscr(uint8_t com);
void UART_setColor(uint8_t com, uint8_t color);  
void UART_gotoxy(uint8_t com, uint8_t x, uint8_t y);


void     itoa(uint16_t number, char* str, uint8_t base);  
uint16_t atoi(char *str);                               

#endif 
