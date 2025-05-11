/*
 * Project_MASTER_ATMEGA2560.c
 *
 * Created: 5.4.2025 3.30.18
 * Authors : Jeremias Nousiainen, Lauri Heininen, Otto Åhlfors, Juhani Juola 
 *
 * 
 */

// Arduino Mega Master device
// I2C/TWI is used to communicate between Master and Slave

#define F_CPU 16000000UL
#define FOSC 16000000UL
#define BAUD 9600
#define MYUBBR (FOSC / 16 / BAUD - 1) // baud rate register value
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

ElevatorState state = IDLE; //Setting elevator state to IDLE

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
{
	/* Wait until the transmit buffer is empty*/
	while (!(UCSR0A & (1 << RXC0)))
	{
		;
	}

	/* Get the received data from the buffer */
	return UDR0;
}

FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE); //Creating a file object uart_output
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);  //Creating a file object uart_input

// Sends 1 byte command to the slave over I2C
void sendCommandToSlave(uint8_t command)
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // Send START
	while (!(TWCR & (1 << TWINT))) //Waiting for START condition to transmit (when TWINT becomes 1)
	{
		;
	}

	TWDR = (SLAVE_ADDRESS << 1); // SLA+W   Left shifting slave address for R/W bit
	TWCR = (1 << TWINT) | (1 << TWEN); //Send address 
	while (!(TWCR & (1 << TWINT))) //Wait for the end of transmission
	{
		;
	}

	TWDR = command; // Send command byte
	TWCR = (1 << TWINT) | (1 << TWEN); //Send address 
	while (!(TWCR & (1 << TWINT))) //Wait for the end of transmission
	{
		;
	}

	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO); // Send STOP
}

void displayFloorMessage(const char *format, int floorNumber, const char *doorOpen) //Display floor and status
{
    char message[50]; //Define a string
    sprintf(message, format, floorNumber); //Format a string with sprintf
    write_to_lcd(message, doorOpen); //Writing floor and status on screen
}

void handleEmergency(int currentFloor, char *doorOpen) //Create a emergency handling state function
{
    uint8_t escape = 0; //Initializing variable to 0

    displayFloorMessage("EMERGENCY %d", currentFloor, doorOpen); //Show emergency message
    sendCommandToSlave(0x03);// Blink movement LED = FAULT
    while (1) //Enter loop
    {
        escape = handleEmergencyKey(); //Emergency key handling
        if (escape == 1) //If emergency key is pressed
        {
            strcpy(doorOpen, "Door open"); //Open door
			displayFloorMessage("EMERGENCY %d", currentFloor, doorOpen); //Display message of emergency
			sendCommandToSlave(0x06);// Play buzzer melody
			strcpy(doorOpen, "Door closed"); //Close door
            state = IDLE; //Set state to IDLE
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

	TWCR |= (1 << TWEN); // Set to enable the TWI

	DDRA &= ~(1 << PA0); // Emergency button input
	uint8_t emergency_button = 0; //Initializing
    char message[50]; //Setting up door closing message
    char doorOpen[15] = "Door closed"; //Creating door closing message
    
	while (1) //Creating a loop
	{


		switch (state) //Create states for elevator
		{
		case IDLE:  //IDLE state waits for input and displays floor
            displayFloorMessage("Floor %d", currentFloor, doorOpen); //Display floor
			selectedFloor = handle_keypad_input(); // Updates keypad buffer
            printf("Floornumber"); // Debuggin test prints
            printf("%d\n",selectedFloor); //Display selected floor
			if (selectedFloor >= 0 && selectedFloor <= 99) //If selected floor number is valid
			{
				state = FLOOR_SELECTED; //set state to FLOOR_SELECTED
			}
			break;

        case FLOOR_SELECTED: //When floor is selected
        if (selectedFloor == currentFloor) //If selected floor is the same as current floor
        {
            sendCommandToSlave(0x03); // Blink movement LED = FAULT
            displayFloorMessage("Already on %d", currentFloor, doorOpen); //Display message
            _delay_ms(2000); //Wait for 2 seconds
            state = DOOR_OPEN; //Open door
        }
        else //Move the elevator to selected floor
        {
            sendCommandToSlave(0x01); // Turn on movement LED
            displayFloorMessage("Moving to %d", selectedFloor, doorOpen); //Display message of moving
           
            _delay_ms(3000); // Simulate movement delay

            while (1) //Simulate elevator moving
            {

                displayFloorMessage("Current floor %d", currentFloor, doorOpen); //Display floor number of passed floors
                _delay_ms(150); //Delay of 150 ms 

                if (currentFloor < selectedFloor) //If below selected floor
                {
                    currentFloor++; // Elevator goes up
                }
                else if (currentFloor > selectedFloor) //If above selected floor
                {
                    currentFloor--; //Elevator goes down
                }
                else if (currentFloor == selectedFloor) //If floor is correct
                {
					sendCommandToSlave(0x02); // Turn off movement LED
					displayFloorMessage("Arrived on %d", currentFloor, doorOpen); // Display message of arrival
					_delay_ms(500); // Delay of half a second
					state = DOOR_OPEN; //Open doors
                    break;
                }
                emergency_button = (PINA & (1 << PA0)); // Emergency check
                if (emergency_button) //If emergency button is pressed
                {   
                    handleEmergency(currentFloor, doorOpen); //Call emergency handling function
					_delay_ms(5000); //Wait for 5 seconds
					state = IDLE; //Set state to IDLE
                    break;
                }

            }  
        }
        break;

		case DOOR_OPEN: // Door-opening sequence
			_delay_ms(100); // wait for 0,1 seconds
            strcpy(doorOpen,"Door open"); // Copy door opening message to string
            displayFloorMessage("Arrived on %d", currentFloor, doorOpen); // DIsplay message of arrival
			sendCommandToSlave(0x04); // Open door LED
			_delay_ms(5000);		  // Hold door open
			sendCommandToSlave(0x05); // Close door LED
            strcpy(doorOpen, "Door closed"); //Copy door closing message to string
			state = IDLE; // Set state to IDLE
			break;

		default:
			state = IDLE; // Set state to IDLE
		}
	}

	return 0;
}
