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
 * \file debug.c
 * \brief Un-buffered UART driver for debug IO
 *
 * Debug UART driver for ATMEL AVR ATMEGA.  Provides
 * integration with stdio and un-buffered input/output.
 *
 * (C) 2008 Mike Stirling
 *
 * $Revision$
 * $Author$
 * $Date$
 */
/*********************************************************
 * $Id$
 * $HeadURL$
 *********************************************************/

#include <avr/io.h>
#include <stdio.h>

#include "board.h"
#include "common.h"
#include "debug.h"

/// \todo This logic is not extensively tested.  There is similar logic in uart.c
/// which provides buffered serial IO and is used elsewhere.

#if (defined (__AVR_ATmega8__) || defined (__AVR_ATmega16__) || defined (__AVR_ATmega32__))

// Port is an old style UART
#define DEBUG_IS_UART		1
#define _UDR				UDR
#define _UBRRL				UBRRL
#define _UBRRH				UBRRH
#define _UCSRA				UCSRA
#define _UCSRB				UCSRB
#define _UCSRC				UCSRC

#elif (defined(__AVR_ATmega128__) \
 || defined (__AVR_ATmega1280__) || defined (_AVR_ATmega1281__) \
 || defined (__AVR_ATmega2560__) || defined (__AVR_ATmega2561__) \
 || defined(__AVR_ATmega164P__) || defined(__AVR_ATmega324P__) \
 || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega48__) \
 || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__))

// Port is a new style USART
#define DEBUG_IS_UART		0

#if DEBUG_PORT == 0
#define _UDR				UDR0
#define _UBRRL				UBRR0L
#define _UBRRH				UBRR0H
#define _UCSRA				UCSR0A
#define _UCSRB				UCSR0B
#define _UCSRC				UCSR0C
#elif DEBUG_PORT == 1
#define _UDR				UDR1
#define _UBRRL				UBRR1L
#define _UBRRH				UBRR1H
#define _UCSRA				UCSR1A
#define _UCSRB				UCSR1B
#define _UCSRC				UCSR1C
#else
# warning "Selected port number not supported by <debug.c>"
#endif

#else
# warning "CPU type not supported by <debug.c>"
#endif

#if DEBUG_IS_STDIO
/// Wrapper function for interfacing with stdio fdevopen.
/// Replaces line-feed characters with a
/// carriage return/line-feed pair and add to transmit buffer.
/// Other characters are passed through unaffected.
/// /param c Character to be transmitted
/// /param f Dummy file pointer
/// /return Always 0.  Dummy return value to allow the character to be used with avrlib stdio
static int debug_writec(char c, FILE *f)
{
	if (c == '\n') {
		debug_putc('\r');
	}
	debug_putc(c);
	return 0;
}

/// Wrapper function for interfacing with stdio fdevopen.
/// Provides blocking character read.
/// /param f Dummy file pointer
/// /return Character that was read
static int debug_readc(FILE *f)
{
	return (int)debug_getc();
}

// Assign streams (stdin and stdout/stderr)
static FILE debug_stdin = FDEV_SETUP_STREAM(NULL,debug_readc,_FDEV_SETUP_READ);
static FILE debug_stdout = FDEV_SETUP_STREAM(debug_writec,NULL,_FDEV_SETUP_WRITE);
#endif

void debug_init(void)
{
	// Configure and enable the UART according to the setup
	// defined in debug.h
#if DEBUG_IS_UART
	#if DEBUG_HIGHSPEED
		_UCSRA = _BV(U2X);
	#else
		_UCSRA = 0;
	#endif

		_UCSRB = 0;
		_UCSRC = _BV(URSEL);

	#if DEBUG_SYNC
		_UCSRC |= _BV(UMSEL);
	#endif

	#if (DEBUG_STOPBITS == 1)
	#elif (DEBUG_STOPBITS == 2)
		_UCSRC |= _BV(USBS);
	#else
	# error Invalid number of stop bits specified in debug.h
	#endif

	#if (DEBUG_DATABITS == 5)
	#elif (DEBUG_DATABITS == 6)
		_UCSRC |= _BV(UCSZ0);
	#elif (DEBUG_DATABITS == 7)
		_UCSRC |= _BV(UCSZ1);
	#elif (DEBUG_DATABITS == 8)
		_UCSRC |= _BV(UCSZ0) | _BV(UCSZ1);
	#else
	# error Invalid number of data bits specified in debug.h
	#endif

	#if (DEBUG_PARITY == 0)
	#elif (DEBUG_PARITY == 1)
		_UCSRC |= _BV(UPM0) | _BV(UPM1);
	#elif (DEBUG_PARITY == 2)
		_UCSRC |= _BV(UPM1);
	#else
	# error Invalid parity setting specified in debug.h
	#endif
#else
	#if DEBUG_HIGHSPEED
		_UCSRA = _BV(U2X0);
	#else
		_UCSRA = 0;
	#endif

		_UCSRB = 0;
		_UCSRC = 0;

	#if DEBUG_SYNC
		_UCSRC |= _BV(UMSEL00);
	#endif

	#if (DEBUG_STOPBITS == 1)
	#elif (DEBUG_STOPBITS == 2)
		_UCSRC |= _BV(USBS0);
	#else
	# error Invalid number of stop bits specified in debug.h
	#endif

	#if (DEBUG_DATABITS == 5)
	#elif (DEBUG_DATABITS == 6)
		_UCSRC |= _BV(UCSZ00);
	#elif (DEBUG_DATABITS == 7)
		_UCSRC |= _BV(UCSZ01);
	#elif (DEBUG_DATABITS == 8)
		_UCSRC |= _BV(UCSZ00) | _BV(UCSZ01);
	#else
	# error Invalid number of data bits specified in debug.h
	#endif

	#if (DEBUG_PARITY == 0)
	#elif (DEBUG_PARITY == 1)
		_UCSRC |= _BV(UPM00) | _BV(UPM01);
	#elif (DEBUG_PARITY == 2)
		_UCSRC |= _BV(UPM01);
	#else
	# error Invalid parity setting specified in uart.h
	#endif
#endif
	_UBRRL = DEBUG_BAUDCONST & 0xff;
	_UBRRH = DEBUG_BAUDCONST >> 8;

#if DEBUG_IS_STDIO
	// Map stdio
	stdout = &debug_stdout;
	stderr = &debug_stdout;
	stdin = &debug_stdin;
#endif

	// Enable transceiver
#if DEBUG_IS_UART
	_UCSRB = _BV(TXEN) | _BV(RXEN);
#else
	_UCSRB = _BV(TXEN0) | _BV(RXEN0);
#endif
}

char debug_getc(void)
{
	// Wait for data to be available
#if DEBUG_IS_UART
	while (! (_UCSRA & _BV(RXC)));
#else
	while (! (_UCSRA & _BV(RXC0)));
#endif
	return _UDR;
	
}

void debug_putc(char c)
{
	// Wait for transmitter to become ready
#if DEBUG_IS_UART
	while (! (_UCSRA & _BV(UDRE)));
#else
	while (! (_UCSRA & _BV(UDRE0)));
#endif
	_UDR = c;
}
