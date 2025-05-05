/*
 * keypad_handler.c
 *
 * Created: 20.4.2025 12.47.09
 *  Author: Otto Åhlfors
 */

#include "keypad_handler.h"
#include "lcd_handler.h" // So you can call write_to_lcd()

int handleEmergencyKey(void)
{
    while (1)
    {
        uint8_t key_signal = KEYPAD_GetKey();

        if (key_signal != 0xFF)
        {
            return 1;
        }
    }
}

int handle_keypad_input(void)
{
	char floor[3] = {0}; // to store two digits and null-terminator
	uint8_t index = 0;

	while (1)
	{
		uint8_t key_signal = KEYPAD_GetKey();
		if (key_signal != 0xFF && key_signal >= '0' && key_signal <= '9')
		{
			if (index < 2)
			{
    			floor[index++] = key_signal;
			}
			else
			{
    			// More than 2 digits entered: reset
    			floor[0] = '0';
    			floor[1] = '\0';
    			index = 0;
			}
            
            write_to_lcd("Choose floor:",floor);
			_delay_ms(300); // debounce delay
			
		}
		else if (key_signal == '#')
		{
			// '#' used to confirm entry
			break;
		}
	}

	if (index > 0)
	{
		return atoi(floor); // convert collected digits to int
        
	}

	return 0; // invalid or no input
}
