/*
 * lcd_handler.h
 *
 * Created: 20.4.2025 12.39.36
 *  Author: Otto Åhlfors
 */ 

#ifndef LCD_HANDLER_H
#define LCD_HANDLER_H

#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h" // lcd header file made by Peter Fleury
#include "keypad.h"

void lcd_setup(void);

void write_to_lcd(const char* line1, const char* line2);

#endif
