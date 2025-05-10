/*
 * Project_SLAVE_ATMEGA328p.c
 *
 * Created: 9.4.2025 2.37.47
 * Author : Jeremias Nousiainen
 *
 * Modified: 30.4.2025 , Lauri Heininen
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

static void USART_Transmit(unsigned char data, FILE *stream) //Declaring a static function for a static byte
{ // p. 207
	 /* Wait until the transmit buffer is empty*/
    while (!(UCSR0A & (1 << UDRE0))) {;} //datasheet p.207, p. 219
 
	/* Puts the data into a buffer, then sends/transmits the data */
	UDR0 = data;
}

static char USART_Receive(FILE *stream) //Defining a character input function for UART. datasheet p.210, 219
{
	/* Wait until the transmit buffer is empty*/
    while (!(UCSR0A & (1 << RXC0))) {;}

    /* Get the received data from the buffer */
	return UDR0;
}

// Emergency sequence
int emergency(void)
{
    
	TCNT1 = 0; // Reset timer to zero
	TCCR1B = 0; // Resets timer counter register to zero
	    
	TCCR1A |= (1 << 6); // Sets to output mode compare match
	    
	TCCR1A |= (1 << 0); // Sets counter to Normal mode s. 145, 154
	TCCR1B |= (1 << 4); // Waveform generation mode
	    
	TIMSK1 |= (1 << 1); // enable counter interrupt
	     
	sei();   // Enabling global interrupts (Set Enable Interrupts)
	
	PORTB |= (1 << PB5); // Emergency led 
    _delay_ms(1000); //Delay program by 1 second.
    OCR1A = 3968; // note C4 252 Hz, no prescaler  s. 151
	TCCR1B |= (1 << 1); // Prescaler set to 8
    PORTB &= ~(1 << PB5); //Set pin PB5 to high
	_delay_ms(1000); //Delay by 1 second
	OCR1A = 4848; //Change tone
    PORTB |= (1 << PB5); // set pin PB 5 to high
	_delay_ms(1000); //Delay by 1 second
	OCR1A = 6000;  //Play a note
    PORTB &= ~(1 << PB5);  //Set pin PB5 to high
	_delay_ms(1000); //Delay by 1 second
	OCR1A = 1000;  //Change tone
    PORTB |= (1 << PB5);  //Set port PB5 to high
	_delay_ms(1000); //Delay by 1 second
	OCR1A = 0;  //Stop playing a note
    PORTB &= ~(1 << PB5); //Set port PB5 to high
	TCNT1 = 0;  //Reset timer1 counter to 1
}

ISR (TIMER1_COMPA_vect) {}  //Setting up Timer1 compare match A interrupt

FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE); //Defining custom output for stdio functions
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ); //Defining custom input for stdio functions

int main(void)
{
    DDRB |= (1 << PB0);  // Movement LED
    DDRD |= (1 << PD7);  // Door LED OTA JOKU
    DDRB |= (1 << PB5);  // Emergency LED
    DDRB |= (1 << PB1);  // Buzzer (if used separately)

    USART_init(MYUBBR);   // Initializing the USART peripheral with a baud rate.
    stdout = &uart_output; //Redirecting the output stream to use the UART.
    stdin = &uart_input;  // Redirecting input to read from the UART.

    // Setup I²C as slave
    TWAR = (SLAVE_ADDRESS << 1);             // Slave address
    TWCR = (1 << TWEA) | (1 << TWEN);        // Enable TWI + ACK

    uint8_t twi_receive_data = 0; // Comes from Master
	uint8_t twi_send_data = 1; // Data sent to Master
    uint8_t twi_status = 0; //Setting the status to 0.

    while (1) {
        while (!(TWCR & (1 << TWINT))) {;}   // Wait for TWI interrupt flag

        twi_status = (TWSR & 0xF8); // Mask prescaler bits

        if ((twi_status == 0x80) || (twi_status == 0x90)) {  // Checking the TWI status code.
            twi_receive_data = TWDR; // Reading the received byte from the TWI data register and storing it in a variable.
            printf("%d\n", twi_receive_data); // Print the received byte

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
                        PORTB |= (1 << PB0);
                        _delay_ms(300);
                        PORTB &= ~(1 << PB0);
                        _delay_ms(300);
                    }
                    break;
                case 0x04: // Door LED ON
                    PORTD |= (1 << PB7);
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
            twi_receive_data = TWDR;
            printf("%d\n", twi_receive_data);
        }
        else if (twi_status == 0xA8) {
            // Slave transmitter mode
            TWDR = 1; // Dummy response
        }
        else if (twi_status == 0xA0) {
            // STOP condition received
            TWCR |= (1 << TWINT);
        }

        TWCR |= (1 << TWINT) | (1 << TWEA) | (1 << TWEN); // Resume listening
    }

    return 0;
}
