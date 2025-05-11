/*
 * lcd_handler.c
 *
 * Created: 20.4.2025 12.39.02
 *  Author: Otto Åhlfors
 */

#include "lcd_handler.h"

// These functions are based on LUT Inroduction To Embeded Systems course Exercise 3 example solution

void lcd_setup()
{
	lcd_init(LCD_DISP_ON); // Initialize LCD with display on, cursor off
	lcd_clrscr();		   // Clear the LCD screen
	lcd_puts("Ready");	   // Display the message "Ready" on the LCD
	_delay_ms(1000);	   // Wait for 1 second to allow the user to see the message
	KEYPAD_Init();		   // Initialize the keypad for user input
}

// This
void write_to_lcd(const char *line1, const char *line2)
{
	lcd_clrscr();	  // Clear the LCD screen before writing new content
	lcd_gotoxy(0, 0); // Set cursor to the beginning of the first line
	lcd_puts(line1);  // Display the first line of text
	lcd_gotoxy(0, 1); // Set cursor to the beginning of the second line
	lcd_puts(line2);  // Display the second line of text
}