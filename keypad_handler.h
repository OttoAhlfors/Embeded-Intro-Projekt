/*
 * keypad_handler.h
 *
 * Created: 20.4.2025 12.47.25
 *  Author: Otto Åhlfors
 */ 

#ifndef KEYPAD_HANDLER_H
#define KEYPAD_HANDLER_H

#include <avr/io.h>
#include <util/delay.h>
#include "keypad.h"
#include <stdlib.h>

/**
 * @brief Handles keypad input and updates the LCD with the key pressed.
 */
void handle_keypad_input(void);

#endif // KEYPAD_HANDLER_H
