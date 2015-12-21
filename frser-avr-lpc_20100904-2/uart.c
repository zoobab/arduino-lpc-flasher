/*
 * This file is part of the frser-avr-lpc project.
 *
 * Copyright (C) 2010 Mike Stirling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 * 
 * \file uart.c
 * \brief Buffered, interrupt driven UART and debugging functions
 * 
 * UART driver for ATMEL AVR ATMEGA.  Provides buffered IO
 * and integration with stdio.
 * 
 * \author	(C) 2004-2007 Mike Stirling 
 * 			($Author$)
 * \version	$Revision$
 * \date	$Date$
 */
 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>

#include "uart.h"

/// Allocate receive buffer
static char rxbuffer[RX_BUFFER_SIZE];
/// Allocate transmit buffer
static char txbuffer[TX_BUFFER_SIZE];
/// Points to the current receive buffer head location
static volatile char *rxbufhead;
/// Points to the current receive buffer tail location
static volatile char *rxbuftail;
/// Points to the current transmit buffer head location
static volatile char *txbufhead;
/// Points to the current transmit buffer tail location
static volatile char *txbuftail;
/// Flags
static volatile unsigned char uart_flags;

#if UART_IS_STDIO
static int uart_writec(char c, FILE *f);
static int uart_readc(FILE *f);

// Assign streams
static FILE uart_stdout = FDEV_SETUP_STREAM(uart_writec,NULL,_FDEV_SETUP_WRITE);
static FILE uart_stdin = FDEV_SETUP_STREAM(NULL,uart_readc,_FDEV_SETUP_READ);
#endif

/// Receive interrupt handler. Circularly buffers up incoming data
ISR(USART_RX_vect)
{
	char c = _UDR;
	
	// If the buffer is full then the data will be lost
	if (! (uart_flags & _BV(UART_FLAGS_OVERFLOW))) {
		*rxbufhead++ = c;
		uart_flags |= _BV(UART_FLAGS_RXAVAIL);
		if (rxbufhead == rxbuffer + RX_BUFFER_SIZE)
			rxbufhead = rxbuffer;
		if (rxbufhead == rxbuftail)
			uart_flags |= _BV(UART_FLAGS_OVERFLOW);
	}
}

/// Transmit interrupt handler. Transmit tx buffer contents
ISR(USART_UDRE_vect)
{
	// Check if data is available for transmission
	if ((txbufhead != txbuftail) || (uart_flags & _BV(UART_FLAGS_TXFULL))) {
		_UDR = *txbuftail++;
		uart_flags &= ~_BV(UART_FLAGS_TXFULL);
		if (txbuftail == txbuffer + TX_BUFFER_SIZE)
			txbuftail = txbuffer;
	} else {
		// Buffer empty - disable the interrupt
#if UART_IS_UART
		UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(RXCIE);
#else
		UCSR0B = _BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0);
#endif
	}
}

/// \brief Returns true if a call to uart_getc will not block
/// Checks the receive buffer pointers to determine whether or not data
/// is available for reading.
/// \return TRUE if a read will not block, FALSE else.
unsigned char uart_poll(void)
{
	return (uart_flags & _BV(UART_FLAGS_RXAVAIL));
}

/// \brief Block until the transmit buffer is empty
/// Checks the transmit buffer pointers to determine whether or not unsent
/// data is remaining, and blocks until the buffer is empty.
void uart_flush(void)
{
	while ((txbufhead != txbuftail) || (uart_flags & _BV(UART_FLAGS_TXFULL)));
}

/// Read a single character from the receive buffer, blocking until one becomes
/// available.
/// \return Character read from buffer
char uart_getc(void)
{
	char c;
	
	// Busy wait until data is available
	while (!uart_poll());

	// Start critical section
	cli();
	c = *rxbuftail++;
	uart_flags &= ~_BV(UART_FLAGS_OVERFLOW);
	if (rxbuftail == rxbuffer + RX_BUFFER_SIZE)
		rxbuftail = rxbuffer;
	if (rxbuftail == rxbufhead)
		uart_flags &= ~_BV(UART_FLAGS_RXAVAIL);
	// End critical section
	sei();

	return c;
}

/// Place a single character in the transmit buffer ready for transmission, if
/// necessary, blocking until space becomes available.
/// \param c Character to be transmitted
void uart_putc(char c)
{
	// Busy wait until space is available
	while (uart_flags & _BV(UART_FLAGS_TXFULL));

	// Put the new character in the buffer
	// Start critical section
	cli();
	*txbufhead++ = c;
	if (txbufhead == txbuffer + TX_BUFFER_SIZE)
		txbufhead = txbuffer;
	if (txbufhead == txbuftail)
		uart_flags |= _BV(UART_FLAGS_TXFULL);
	// End critical section
	sei();

	// Enable the transmit interrupt to commence transmitting
#if UART_IS_UART
	UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(RXCIE) | _BV(UDRIE);
#else
	UCSR0B = _BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0) | _BV(UDRIE0);
#endif
}

#if UART_IS_STDIO
/// Wrapper function for interfacing with stdio fdevopen.
/// Replaces line-feed characters with a
/// carriage return/line-feed pair and add to transmit buffer.
/// Other characters are passed through unaffected.
/// \param c Character to be transmitted
/// \param f Dummy file pointer
/// \return Always 0.  Dummy return value to allow the character to be used with avrlib stdio
static int uart_writec(char c, FILE *f)
{
	if (c == '\n') uart_putc('\r');
	uart_putc(c);
	return 0;
}

/// Wrapper function for interfacing with stdio fdevopen.
/// Provides blocking character read.
/// \param f Dummy file pointer
/// \return Character that was read
static int uart_readc(FILE *f)
{
	return (int)uart_getc();
}
#endif

/// Open the UART and assign stdin and stdout if requested
void uart_init(void)
{
	// Initialise the buffers
	uart_flags = 0;
	rxbufhead = rxbuftail = rxbuffer;
	txbufhead = txbuftail = txbuffer;

	// Configure and enable the UART according to the setup
	// defined in uart.h
#if UART_IS_UART
	#if UART_HIGHSPEED
		UCSRA = _BV(U2X);
	#else
		UCSRA = 0;
	#endif

		UCSRB = 0;
		UCSRC = _BV(URSEL);

	#if UART_SYNC
		UCSRC |= _BV(UMSEL);
	#endif

	#if (UART_STOPBITS == 1)
	#elif (UART_STOPBITS == 2)
		UCSRC |= _BV(USBS);
	#else
	# error Invalid number of stop bits specified in uart.h
	#endif

	#if (UART_DATABITS == 5)
	#elif (UART_DATABITS == 6)
		UCSRC |= _BV(UCSZ0);
	#elif (UART_DATABITS == 7)
		UCSRC |= _BV(UCSZ1);
	#elif (UART_DATABITS == 8)
		UCSRC |= _BV(UCSZ0) | _BV(UCSZ1);
	#else
	# error Invalid number of data bits specified in uart.h
	#endif

	#if (UART_PARITY == 0)
	#elif (UART_PARITY == 1)
		UCSRC |= _BV(UPM0) | _BV(UPM1);
	#elif (UART_PARITY == 2)
		UCSRC |= _BV(UPM1);
	#else
	# error Invalid parity setting specified in uart.h
	#endif
#else
	#if UART_HIGHSPEED
		UCSR0A = _BV(U2X0);
	#else
		UCSR0A = 0;
	#endif

		UCSR0B = 0;
		UCSR0C = 0;

	#if UART_SYNC
		UCSR0C |= _BV(UMSEL00);
	#endif

	#if (UART_STOPBITS == 1)
	#elif (UART_STOPBITS == 2)
		UCSR0C |= _BV(USBS0);
	#else
	# error Invalid number of stop bits specified in uart.h
	#endif

	#if (UART_DATABITS == 5)
	#elif (UART_DATABITS == 6)
		UCSR0C |= _BV(UCSZ00);
	#elif (UART_DATABITS == 7)
		UCSR0C |= _BV(UCSZ01);
	#elif (UART_DATABITS == 8)
		UCSR0C |= _BV(UCSZ00) | _BV(UCSZ01);
	#else
	# error Invalid number of data bits specified in uart.h
	#endif

	#if (UART_PARITY == 0)
	#elif (UART_PARITY == 1)
		UCSR0C |= _BV(UPM00) | _BV(UPM01);
	#elif (UART_PARITY == 2)
		UCSR0C |= _BV(UPM01);
	#else
	# error Invalid parity setting specified in uart.h
	#endif
#endif
	_UBRR = UART_BAUDCONST;

#if UART_IS_STDIO
	// Map stdio
	stdout = &uart_stdout;
	stderr = &uart_stdout;
	stdin = &uart_stdin;
#endif

	// Enable rx interrupts - main code must globally enable interrupts
#if UART_IS_UART
	UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(RXCIE);
#else
	UCSR0B = _BV(TXEN0) | _BV(RXEN0) | _BV(RXCIE0);
#endif	
}
