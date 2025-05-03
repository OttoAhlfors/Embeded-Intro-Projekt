/*
 * lcd_handler.c
 *
 * Created: 20.4.2025 12.39.02
 *  Author: Otto Åhlfors
 */ 

#include "lcd_handler.h"

void lcd_setup() {
	lcd_init(LCD_DISP_ON);
	lcd_clrscr();
	lcd_puts("Ready");
	_delay_ms(1000);
	KEYPAD_Init();
}

void write_to_lcd(const char* line1, const char* line2) {
	lcd_clrscr();
	lcd_gotoxy(0, 0);
	lcd_puts(line1);
	lcd_gotoxy(0, 1);
	lcd_puts(line2);
}