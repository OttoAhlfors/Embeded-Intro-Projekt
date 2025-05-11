/*
 * keypad_handler.c
 *
 * Created: 20.4.2025 12.47.09
 *  Author: Otto Åhlfors
 */

#include "keypad_handler.h"
#include "lcd_handler.h" // So you can call write_to_lcd()

// This function is used to wait for keypad input in case of emergencies
int handleEmergencyKey(void)
{
	while (1)
	{
		uint8_t key_signal = KEYPAD_GetKey(); // This function returns the pressed keypad key

		// If any valid key is pressed return 1 to exit loop
		if (key_signal != 0xFF)
		{
			return 1;
		}
	}
}

// This function is based on LUT Inroduction To Embeded Systems course Exercise 3 example solution
// This function handles the input loop for entering floors
int handle_keypad_input(void)
{
	char floor[3] = {0}; // to store two digits and null-terminator
	uint8_t index = 0;

	while (1)
	{
		uint8_t key_signal = KEYPAD_GetKey(); // This function returns the pressed keypad key

		if (key_signal != 0xFF && key_signal >= '0' && key_signal <= '9') // Accept all number keys with value between 0 and 9
		{
			if (index < 2) // if there are less that 2 numbers selected
			{
				floor[index++] = key_signal; // add the the pressed key to to the selected floor
			}
			else
			{
				// If more than 2 digits entered set the floor to 0
				floor[0] = '0';
				floor[1] = '\0';

				// Start the floor selection over again
				index = 0;
			}

			write_to_lcd("Choose floor", floor);
			_delay_ms(300); // debounce delay
		}
		else if (key_signal == '#') // '#' used to confirm entry of floor
		{
			break;
		}
	}

	if (index > 0)
	{
		return atoi(floor); // convert collected digits to int
	}

	return 0; // invalid or no input
}
