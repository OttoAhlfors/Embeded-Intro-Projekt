/*
 * Project_SLAVE_ATMEGA328p.c
 *
 * Created: 9.4.2025 2.37.47
 * Authors : Jeremias Nousiainen, Lauri Heininen, Otto Åhlfors, Juhani Juola
 *
 */ 
// Arduino UNO Slave device

#define F_CPU 16000000UL
#define FOSC 16000000UL
#define BAUD 9600

#define MYUBBR (FOSC/16/BAUD-1) // baud rate register value, datasheet p.203, 226
#define SLAVE_ADDRESS 0b1010111 // 87 as decimal. Address must be same as masters address

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <avr/interrupt.h>


// USART setup for debug output (unchanged)
static void USART_init(uint16_t ubrr)
{
    UBRR0H = (unsigned char)(ubrr >> 8); // set baud rate in UBBR0H and UBBR0L p. 206
    UBRR0L = (unsigned char)ubrr; //Setting up the UART
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0); // enable receiver and transmitter in USART register B p. 206, 220
    UCSR0C |= (1 << USBS0) | (3 << UCSZ00); // frame format in UCSRnC
}

static void USART_Transmit(unsigned char data, FILE *stream)
{ // p. 207
	 /* Wait until the transmit buffer is empty*/
    while (!(UCSR0A & (1 << UDRE0))) {;} //datasheet p.207, p. 219
 
	/* Puts the data into a buffer, then sends/transmits the data */
	UDR0 = data;
}

static char USART_Receive(FILE *stream) //datasheet p.210, 219
{
	/* Wait until the transmit buffer is empty*/
    while (!(UCSR0A & (1 << RXC0))) {;}

    /* Get the received data from the buffer */
	return UDR0;
}

// Emergency sequence
void emergency(void)
{
    
	TCNT1 = 0; // Reset timer to zero
	TCCR1B = 0; // Resets timer counter register to zero
	    
	TCCR1A |= (1 << 6); // Sets to output mode compare match
	    
	TCCR1A |= (1 << 0); // Sets counter to Normal mode
	TCCR1B |= (1 << 4); // Waveform generation mode
	    
	TIMSK1 |= (1 << 1); // Enable counter interrupt
	     
	sei();
	PORTD |= (1 << PD7); // Dooropen LED
    TCCR1B |= (1 << 1); // Prescaler set to 8
    
    uint16_t note = 1000; // Declaring a variable for emergency note
    PORTD |= (1 << PD7); // Dooropen

    for (int i = 0; i < 6; i++) // Loop for playing emergency note
    {
        OCR1A = note; // Note frequency
        note = note+750; //Changing the note frequency
        _delay_ms(500); //Wait for half a second
    }    
         
	// Disable all previously set settings
    TCCR1A &= ~(1 << 6); //Clears COM1A
    TCCR1A &= ~(1 << 0); //Clears WGM10
    TCCR1B &= ~(1 << 4); //Clears WGM12
    TIMSK1 &= ~(1 << 1); //Clears OCIE1A

	//Resetting timer registers
	OCR1A = 0; //Set Output Compare register to 0
	TCNT1 = 0; //Reset Counter1
	_delay_ms(2000); //Wait for 2 seconds
    PORTD &= ~(1 << PD7); // Close door
}

ISR (TIMER1_COMPA_vect) {} //Define an interrupt service routine for Timer1

FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE); //Defining custom output for stdio commands
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ); //Defining custom input for stdio commands

int main(void)
{
    DDRB |= (1 << PB0);  // Movement LED
    DDRD |= (1 << PD7);  // Door LED
    DDRB |= (1 << PB5);  // Emergency LED
    DDRB |= (1 << PB1);  // Buzzer

    USART_init(MYUBBR);  // Initializing the USART peripheral with a baud rate.
    stdout = &uart_output; //Redirecting the output stream to use the UART.
    stdin = &uart_input; // Redirecting input to read from the UART

    // Setup I²C as slave
    TWAR = (SLAVE_ADDRESS << 1);             // Slave address
    TWCR = (1 << TWEA) | (1 << TWEN);        // Enable TWI + ACK

    uint8_t twi_receive_data = 0; // Comes from Master
	uint8_t twi_send_data = 1; // Data sent to Master
    uint8_t twi_status = 0; 

    while (1) {
        while (!(TWCR & (1 << TWINT))) {;}   // Wait for TWI interrupt flag

        twi_status = (TWSR & 0xF8); // Mask prescaler bits

        if ((twi_status == 0x80) || (twi_status == 0x90)) { //If data has been received and ACK returned
            twi_receive_data = TWDR; //Store the received data in twi_receive_data
            printf("%d\n", twi_receive_data); // Transmission test print

            // React to master's command
            switch (twi_receive_data) {
                case 0x01: // Movement LED ON
                    PORTB |= (1 << PB0);
                    break;
                case 0x02: // Movement LED OFF
                    PORTB &= ~(1 << PB0);
                    break;
                case 0x03: // Blink movement LED 3x (FAULT)
                    for (int i = 0; i < 3; i++) {
                        PORTB |= (1 << PB0); //Turn on LED
                        _delay_ms(300);
                        PORTB &= ~(1 << PB0); //Turn off LED
                        _delay_ms(300);
                    }
                    break;
                case 0x04: // Door LED ON
                    PORTD |= (1 << PD7); 
                    break;
                case 0x05: // Door LED OFF
                    PORTD &= ~(1 << PD7);
                    break;
                case 0x06: // Emergency routine
                    emergency();
                    break;
            }
        }
        else if ((twi_status == 0x88) || (twi_status == 0x98)) {
            // Data received but NACK'd
            twi_receive_data = TWDR; //Read the received byte
            printf("NACK"); //Print "NACK"
            printf("%d\n", twi_receive_data); //Print the received byte
        }
        else if (twi_status == 0xA0) {
            // STOP condition received
            TWCR |= (1 << TWINT); //Setting a TWINT flag
        }

        TWCR |= (1 << TWINT) | (1 << TWEA) | (1 << TWEN); // Resume listening
    }

    return 0;
}
