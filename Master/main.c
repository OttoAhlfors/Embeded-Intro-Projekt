/*
 * Project_MASTER_ATMEGA2560.c
 *
 * Created: 5.4.2025 3.30.18
 * Author : Jeremias Nousiainen
 *
 * Modified: 30.4.2025, Lauri Heininen
 */

// Arduino Mega Master device
// I2C/TWI is used to communicate between Master and Slave

#define F_CPU 16000000UL
#define FOSC 16000000UL
#define BAUD 9600
#define MYUBBR (FOSC / 16 / BAUD - 1) // baud rate register value, datasheet p.203, 226
#define SLAVE_ADDRESS 0b1010111		  // 87 as decimal

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <stdbool.h>

// These include functions to handle lcd and keypad
#include "lcd_handler.h"
#include "keypad_handler.h"

// Elevator FSM states
typedef enum
{
	IDLE,
	FLOOR_SELECTED,
	MOVING,
	FAULT,
	DOOR_OPEN
} ElevatorState;

ElevatorState state = IDLE;

// Track current and selected floor
uint8_t currentFloor = 1;
uint8_t selectedFloor = 1;

// USART init for debugging via serial
static void USART_init(uint16_t ubrr)
{
	UBRR0H = (unsigned char)(ubrr >> 8); // set baud rate in UBBR0H and UBBR0L p. 206
	UBRR0L = (unsigned char)ubrr;

	UCSR0B |= (1 << RXEN0) | (1 << TXEN0);	// enable receiver and transmitter in USART register B p. 206, 220
	UCSR0C |= (1 << USBS0) | (3 << UCSZ00); // frame format in UCSRnC
}

// USART transmit helper
static void USART_Transmit(unsigned char data, FILE *stream)
{ // p. 207
	/* Wait until the transmit buffer is empty*/
	while (!(UCSR0A & (1 << UDRE0)))
	{
		;
	}

	/* Puts the data into a buffer, then sends/transmits the data */
	UDR0 = data;
}

// USART receive helper
static char USART_Receive(FILE *stream)
{ // datasheet p.210, 219
	/* Wait until the transmit buffer is empty*/
	while (!(UCSR0A & (1 << RXC0)))
	{
		;
	}

	/* Get the received data from the buffer */
	return UDR0;
}

FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

// Sends 1 byte command to the slave over I2C
void sendCommandToSlave(uint8_t command)
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // Send START
	while (!(TWCR & (1 << TWINT)))
	{
		;
	}

	TWDR = (SLAVE_ADDRESS << 1); // SLA+W
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
	{
		;
	}

	TWDR = command; // Send command byte
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)))
	{
		;
	}

	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO); // Send STOP
}

void displayFloorMessage(const char *format, int floorNumber, const char *doorOpen)
{
    char message[50];
    sprintf(message, format, floorNumber);
    write_to_lcd(message, doorOpen);
}

void handleEmergency(int currentFloor, char *doorOpen)
{
    uint8_t escape = 0;

    displayFloorMessage("EMERGENCY %d", currentFloor, doorOpen);
    sendCommandToSlave(0x03);// Blink movement LED = FAULT
    while (1)
    {
        escape = handleEmergencyKey();
        if (escape == 1)
        {
            //state = DOOR_OPEN;
            sendCommandToSlave(0x06);// Play buzzer melody
            state = IDLE;
            break;
        }
    }
}

int main(void)
{   
    lcd_clrscr();
	USART_init(MYUBBR); // initialize USART with 9600 Baud
	lcd_setup();		// initialize lcd

	stdout = &uart_output; // redirect stdin/out to UART function
	stdin = &uart_input;

	// Initialize TWI/I²C
	TWBR = 0x03; // TWI bit rate register.
	TWSR = 0x00; // TWI status register prescaler value set to 1

	TWCR |= (1 << TWEN); // Must be set to enable the TWI, datasheet p. 248

	DDRA &= ~(1 << PA0); // Emergency button as input
	uint8_t emergency_button = 0;
    char message[50];
    char doorOpen[15] = "Door closed";
    
	while (1)
	{


		switch (state)
		{
		case IDLE:
            displayFloorMessage("Floor: %d", currentFloor, doorOpen);
			selectedFloor = handle_keypad_input(); // Updates keypad buffer
            printf("t");
            printf("%d\n",selectedFloor);
			if (selectedFloor >= 0 && selectedFloor <= 99)
			{
				state = FLOOR_SELECTED;
			}
			break;

        case FLOOR_SELECTED:
        if (selectedFloor == currentFloor)
        {
            sendCommandToSlave(0x03); // Blink movement LED = FAULT
            displayFloorMessage("Already on %d", currentFloor, doorOpen);
            _delay_ms(2000);
            state = DOOR_OPEN;
        }
        else
        {
            sendCommandToSlave(0x01); // Turn on movement LED
            displayFloorMessage("Moving to %d", selectedFloor, doorOpen);
           
            _delay_ms(3000); // Simulate movement delay

            while (1)
            {

                displayFloorMessage("Current floor %d", currentFloor, doorOpen);
                _delay_ms(150);

                if (currentFloor < selectedFloor)
                {
                    currentFloor++;
                }
                else if (currentFloor > selectedFloor)
                {
                    currentFloor--;
                }
                else if (currentFloor == selectedFloor)
                {
                    break;
                }
                emergency_button = (PINA & (1 << PA0)); // Emergency check
                if (emergency_button)
                {   
                    handleEmergency(currentFloor, doorOpen);
                    break;
                }

            }
            sendCommandToSlave(0x02); // Turn off movement LED
            displayFloorMessage("Arrived on %d", currentFloor, doorOpen);
            
            _delay_ms(1000);
            state = DOOR_OPEN;
        }
        break;



		case DOOR_OPEN:
			_delay_ms(100);
            strcpy(doorOpen,"Door open");
            displayFloorMessage("Arrived on %d", currentFloor, doorOpen); 
			sendCommandToSlave(0x04); // Open door LED
			_delay_ms(5000);		  // Hold door open
			sendCommandToSlave(0x05); // Close door LED
            strcpy(doorOpen, "Door closed");
			state = IDLE;
			break;

		default:
			state = IDLE;
		}
	}

	return 0;
}
