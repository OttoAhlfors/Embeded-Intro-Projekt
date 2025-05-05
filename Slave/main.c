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
    UBRR0L = (unsigned char)ubrr;
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
	    
	TCCR1A |= (1 << 0); // Sets counter to Normal mode s. 145, 154
	TCCR1B |= (1 << 4); // Waveform generation mode
	    
	TIMSK1 |= (1 << 1); // enable counter interrupt
	     
	sei();
	PORTD |= (1 << PD7);
    
    TCCR1B |= (1 << 1); // Prescaler set to 8
    
    uint16_t note = 1000;
    PORTD |= (1 << PD7);
    for (int i = 0; i < 6; i++) // Loop for playing note
    {
        OCR1A = note; // Note frequency
        note = note+750;
        _delay_ms(500);
    }    
         
	// Disable all previously set settings
    TCCR1A &= ~(1 << 6);
    TCCR1A &= ~(1 << 0);
    TCCR1B &= ~(1 << 4);
    TIMSK1 &= ~(1 << 1);

	OCR1A = 0;
	TCNT1 = 0;
    PORTD &= ~(1 << PD7);
}

ISR (TIMER1_COMPA_vect) {}

FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

int main(void)
{
    DDRB |= (1 << PB0);  // Movement LED
    DDRD |= (1 << PD7);  // Door LED OTA JOKU
    DDRB |= (1 << PB5);  // Emergency LED
    DDRB |= (1 << PB1);  // Buzzer (if used separately)

    USART_init(MYUBBR);
    stdout = &uart_output;
    stdin = &uart_input;

    // Setup I²C as slave
    TWAR = (SLAVE_ADDRESS << 1);             // Slave address
    TWCR = (1 << TWEA) | (1 << TWEN);        // Enable TWI + ACK

    uint8_t twi_receive_data = 0; // Comes from Master
	uint8_t twi_send_data = 1; // Data sent to Master
    uint8_t twi_status = 0; 

    while (1) {
        while (!(TWCR & (1 << TWINT))) {;}   // Wait for TWI interrupt flag

        twi_status = (TWSR & 0xF8); // Mask prescaler bits

        if ((twi_status == 0x80) || (twi_status == 0x90)) {
            twi_receive_data = TWDR;
            printf("%d\n", twi_receive_data);

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
                        _delay_ms(200);
                        PORTB &= ~(1 << PB0);
                        _delay_ms(200);
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
