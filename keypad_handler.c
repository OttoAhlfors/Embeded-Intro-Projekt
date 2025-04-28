/*
 * keypad_handler.c
 *
 * Created: 20.4.2025 12.47.09
 *  Author: Otto Åhlfors
 */ 

#include "keypad_handler.h"
#include "lcd_handler.h" // So you can call write_to_lcd()

void handle_keypad_input(void) {
	uint8_t key_signal = KEYPAD_GetKey();
	_delay_ms(300);

	if (key_signal != 0xFF) { // filter out invalid reads
		char key_str[4];

		if (key_signal >= '1' && key_signal <= '9') {
			uint8_t key_value = key_signal - '0';
			itoa(key_value, key_str, 10);
		} else {
			key_str[0] = key_signal;
			key_str[1] = '\0';
		}

		write_to_lcd("Key Pressed:", key_str);
	}
}
