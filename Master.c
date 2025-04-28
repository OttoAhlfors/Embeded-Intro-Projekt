/*
 * Project_MASTER_ATMEGA2560.c
 *
 * Created: 5.4.2025 3.30.18
 * Author : Jeremias Nousiainen
 */ 

// Arduino Mega Master device
// I2C/TWI is used to communicate between Master and Slave


#define F_CPU 16000000UL
#define FOSC 16000000UL
#define BAUD 9600
#define MYUBBR (FOSC/16/BAUD-1) // baud rate register value, datasheet p.203, 226
#define SLAVE_ADDRESS 0b1010111 // 87 as decimal

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <stdbool.h>

// These include functions to handle lcd and keypad
#include "lcd_handler.h"
#include "keypad_handler.h"

static void USART_init(uint16_t ubrr){
    UBRR0H = (unsigned char) (ubrr >> 8); // set baud rate in UBBR0H and UBBR0L p. 206
    UBRR0L = (unsigned char) ubrr;
    
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0); // enable receiver and transmitter in USART register B p. 206, 220
    UCSR0C |= (1 << USBS0) | (3 << UCSZ00); // frame format in UCSRnC
}

static void USART_Transmit(unsigned char data, FILE *stream){ // p. 207
    /* Wait until the transmit buffer is empty*/
    while(!(UCSR0A & (1 << UDRE0))) //datasheet p.207, p. 219 
    {
        ;
    }
    /* Puts the data into a buffer, then sends/transmits the data */
    UDR0 = data;
}

static char USART_Receive(FILE *stream) //datasheet p.210, 219
{
    /* Wait until the transmit buffer is empty*/
    while(!(UCSR0A & (1 << RXC0)))
    {
        ;
    }
    /* Get the received data from the buffer */
    return UDR0;
}

FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);

FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

int datatransfer(uint8_t send, uint8_t status, uint8_t receive){
            // First master transfers and slave reads
            TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // Send Start conditions
            while (!(TWCR & (1 << TWINT))) // Wait for start condition to be transmitted
            {;}
            
            TWDR = 0b10101110; //SLA_W --> 7-bit slave address and 1 write bit//p. 247, p. 263
            // Slave address + write bit '0' as an LSB
            
            TWCR = (1 << TWINT) | (1 << TWEN); // Start conditions
            while (!(TWCR & ( 1<< TWINT)))// start condition transmission
            {;}
            
            // Load data into TWDR
            TWDR = send;
            
            // Clear TWINT bit in the TWCR to start transmission of data
            TWCR = (1 << TWINT) | (1 << TWEN);
            while (!(TWCR & ( 1<< TWINT)))
            {;}

            TWCR = (1 << TWINT) | (1 << TWEN) |(1 << TWSTO); // Stop transfer
            
            // After this the master reads data from slave
            TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); // Start conditions again
            while (!(TWCR & (1 << TWINT)))
            {;}
            
            status = (TWSR & 0xF8);
            
            TWDR = 0b10101111; // 7-bit slave address and 1 READ bit
            TWCR = (1 << TWINT) | (1 << TWEN);
            while (!(TWCR & (1 << TWINT)))
            {;}
            
            TWCR = (1 << TWINT) | (1 << TWEN);
            while (!(TWCR & (1 << TWINT)))
            {;}
            
            receive = TWDR; // Read data from TWDR register
            
            TWCR = (1 << TWINT) | (1 << TWEN) |(1 << TWSTO); // Stop transfer
            
            printf("%d",(int)receive);
            printf("\n");
            _delay_ms(1000);
}

int main(void)
{
    USART_init(MYUBBR); //initialize USART with 9600 Baud
    lcd_setup(); // initialize lcd
    
    stdout = &uart_output; // redirect stdin/out to UART function
    stdin = &uart_input;
    
    TWBR = 0x03; // TWI bit rate register.
    TWSR = 0x00; // TWI status register prescaler value set to 1
    
    TWCR |= (1 << TWEN); //Must be set to enable the TWI, datasheet p. 248
    
    DDRA &= ~(1 << PA0); // emergency button input
    DDRA |= (1 << PA2);
    uint8_t twi_send_data = 0;
    int8_t emergency_button = 0;
    uint8_t twi_received_data;
    uint8_t twi_status = 0;
    
    while (1) {
        
        emergency_button = (PINA & (1 << PA0)); // reads emergency button value
        if(0 != emergency_button){
            twi_send_data = 1;
        } else{
            twi_send_data = 0;
            handle_keypad_input(); // detect kaypad buttons 
        }
        
        // Sends 'twi_send_data' - value to slave, slave does emergency in this case
        datatransfer(twi_send_data, twi_status, twi_received_data); 
        
    }
    return 0;
}


