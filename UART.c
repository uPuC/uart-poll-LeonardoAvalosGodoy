#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include "UART.h"   

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

// Initialization
void UART_Ini(uint8_t com, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop)
{
    if (com > 3 || baudrate == 0) return;

    // Punteros a registros por USARTn
    volatile uint8_t * const UCSRA[] = { &UCSR0A, &UCSR1A, &UCSR2A, &UCSR3A };
    volatile uint8_t * const UCSRB[] = { &UCSR0B, &UCSR1B, &UCSR2B, &UCSR3B };
    volatile uint8_t * const UCSRC[] = { &UCSR0C, &UCSR1C, &UCSR2C, &UCSR3C };
    volatile uint8_t * const UBRRL[] = { &UBRR0L, &UBRR1L, &UBRR2L, &UBRR3L };
    volatile uint8_t * const UBRRH[] = { &UBRR0H, &UBRR1H, &UBRR2H, &UBRR3H };

    // Calcular UBRR para modo normal (16x) y doble (8x), con redondeo
    uint32_t ubrr_n = (F_CPU + (16UL*baudrate)/2) / (16UL*baudrate) - 1UL;
    uint32_t ubrr_d = (F_CPU + ( 8UL*baudrate)/2) / ( 8UL*baudrate) - 1UL;

    // Elegir el que dé menor error
    uint32_t real_n = F_CPU / (16UL * (ubrr_n + 1UL));
    uint32_t real_d = F_CPU / ( 8UL * (ubrr_d + 1UL));
    uint32_t err_n  = (real_n > baudrate) ? (real_n - baudrate) : (baudrate - real_n);
    uint32_t err_d  = (real_d > baudrate) ? (real_d - baudrate) : (baudrate - real_d);
    uint8_t  use_double = (err_d < err_n);

    if (use_double) {
        *UCSRA[com] |=  (1 << U2X0); 
        *UBRRL[com]  =  (uint8_t)(ubrr_d & 0xFF);
        *UBRRH[com]  =  (uint8_t)((ubrr_d >> 8) & 0x0F);
    } else {
        *UCSRA[com] &= ~(1 << U2X0); // modo normal
        *UBRRL[com]  =  (uint8_t)(ubrr_n & 0xFF);
        *UBRRH[com]  =  (uint8_t)((ubrr_n >> 8) & 0x0F);
    }

    // Limpiar formato previo
    *UCSRB[com] &= ~(1 << UCSZ02);
    *UCSRC[com] &= ~((1 << UCSZ01) | (1 << UCSZ00) | (1 << UPM01) | (1 << UPM00) | (1 << USBS0));

  
    if (size < 5 || size > 9) size = 8;
    switch (size) {
        case 6: *UCSRC[com] |= (1 << UCSZ00); break;                             // 001
        case 7: *UCSRC[com] |= (1 << UCSZ01); break;                             // 010
        case 8: *UCSRC[com] |= (1 << UCSZ01) | (1 << UCSZ00); break;             // 011
        case 9: *UCSRC[com] |= (1 << UCSZ01) | (1 << UCSZ00); *UCSRB[com] |= (1 << UCSZ02); break; // 111
        default: 
		break;
    }

    // Paridad: 0=none, 1=odd, 2=even
    if (parity == 1) { // impar
        *UCSRC[com] |= (1 << UPM01) | (1 << UPM00);
    } else if (parity == 2) { // par
        *UCSRC[com] |= (1 << UPM01);
    }

 
    if (stop == 2) *UCSRC[com] |=  (1 << USBS0);
    else           *UCSRC[com] &= ~(1 << USBS0);


    *UCSRB[com] |= (1 << RXEN0) | (1 << TXEN0);
}


// Send
void UART_puts(uint8_t com, char *str){
	while (*str){
	UART_putchar(com, *str);
	str++;
	}

}
void UART_putchar(uint8_t com, char data){
    switch (com){
        case 0:
            while (!(UCSR0A & (1 << UDRE0)));
            UDR0 = data;
            break;
        case 1:
            while (!(UCSR1A & (1 << UDRE1)));
            UDR1 = data;
            break;
        case 2:
            while (!(UCSR2A & (1 << UDRE2)));
            UDR2 = data;
            break;
        case 3:
            while (!(UCSR3A & (1 << UDRE3)));
            UDR3 = data;
            break;
    }
}


uint8_t UART_available(uint8_t com){
    switch (com){
        case 0: return (UCSR0A & (1 << RXC0)) ? 1 : 0;
        case 1: return (UCSR1A & (1 << RXC1)) ? 1 : 0;
        case 2: return (UCSR2A & (1 << RXC2)) ? 1 : 0;
        case 3: return (UCSR3A & (1 << RXC3)) ? 1 : 0;
        default: return 0;
    }
}


char UART_getchar(uint8_t com ){
	switch (com){
	case 0:
		while (!(UCSR0A & ( 1 << RXC0)));
		return UDR0;

	case 1:
		while (!(UCSR1A & ( 1 << RXC1)));
		return UDR1;

	case 2:
		while (!(UCSR2A & ( 1 << RXC2)));
		return UDR2;

	case 3:
		while (!(UCSR3A & ( 1 << RXC3)));
		return UDR3;
	default:
		return 0;
}
}
void UART_gets(uint8_t com, char *str)
{
    if (com > 3) { if (str) str[0] = '\0'; return; }

    uint8_t i = 0;

    UART_puts(com, "\x1B[s");  // guardar cursor
    UART_puts(com, "\x1B[K");  // limpiar hasta fin de línea

    for (;;) {
        char c = UART_getchar(com);

        if (c == '\r' || c == '\n') {
            str[i] = '\0';
            break;

        } else if (c == 8 || c == 127) {       // backspace
            if (i > 0) {
                i--;
                str[i] = '\0';
                UART_puts(com, "\b \b");
            } else {
                UART_puts(com, "\x1B[u");      // restaurar cursor
            }

        } else if (i < 16) {
            str[i++] = c;
            UART_putchar(com, c);              // eco
        }
    }
}

void UART_clrscr(uint8_t com){
    UART_puts(com, "\x1B[2J\x1B[H");
}

void UART_setColor(uint8_t com, uint8_t color){
    char buffer[8];
    sprintf(buffer, "\x1B[%dm", 30 + color);
    UART_puts(com, buffer);
}

void UART_gotoxy(uint8_t com, uint8_t x, uint8_t y){
    char buffer[16];
    sprintf(buffer, "\x1B[%d;%dH", y, x);
    UART_puts(com, buffer);
}


void itoa(uint16_t number, char* str, uint8_t base){
	char buffer[17];
	uint8_t i = 0;

	//validar base (base minima 2, maxima 16)
	if (base < 2 || base > 16){
		str[0] = '\0';
		return;
	}

	if (number == 0){
		str[0] = '0';
		str[1] = '\0';
		return;
	}
	//contruye la cadena en orden inverso
	while (number > 0){
	uint8_t digit = number % base;
	buffer[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
	number /=base;
	}
	//invierte la cadena
	for (uint8_t j = 0; j < i; j++){
	str[j] = buffer[ i - j - 1];
	}
	str[i] = '\0';
}

uint16_t atoi(char *str){
	
	uint16_t result=0;

	while(*str && *str !='.'){
	if (*str >='0' && *str <='9')
	result = (uint16_t)(result * 10 + (*str - '0'));
	else
	break;
	str++;
	}
	return result;
}
