/*!
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
 * \file uart.h
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

#ifndef _UART_H
#define _UART_H

#ifndef F_CPU
# warning "F_CPU not defined for <uart.h>"
# define F_CPU 1000000UL
#endif

/* Module configuration */

/// Desired baud rate
#define UART_BAUD			115200

/// Number of data bits (5,6,7 or 8)
#define UART_DATABITS		8

/// Number of stop bits (1 or 2)
#define UART_STOPBITS		1

/// Parity (0 = none, 1 = odd, 2 = even)
#define UART_PARITY			0

/// Length of receive buffer
#define RX_BUFFER_SIZE 		128

/// Length of transmit buffer
#define TX_BUFFER_SIZE 		32

/// If the UART is to operate in high-speed mode
#define UART_HIGHSPEED		1

/// If the UART is to operate in synchronous mode
#define UART_SYNC			0

/// If the UART is to bind to stdio
#define UART_IS_STDIO		0

/* Internal macros */

/// Divider value for baud rate calculation
#if UART_SYNC
#define UART_DIV		2
#elif UART_HIGHSPEED
#define UART_DIV		8
#else
#define UART_DIV		16
#endif

/// Calculated baud rate constant
#define UART_BAUDCONST	(F_CPU/UART_DIV/UART_BAUD-1)

/// Flag.  Receive buffer is full
#define UART_FLAGS_OVERFLOW	0
/// Flag.  Transmit buffer is full
#define UART_FLAGS_TXFULL	1
/// Flag.  Receive buffer is not empty (stupid workaround for misbehaving volatiles!)
#define UART_FLAGS_RXAVAIL	2

// If stdio stuff is to be built then define the dprintf macro
#ifdef DEBUG
#define UART_IS_STDIO		1
#define dprintf(...) printf_P(__VA_ARGS__)
#else
#define dprintf(...)
#endif

#if UART_IS_STDIO
#include <stdio.h>
#include <avr/pgmspace.h>
#endif

#if (defined (__AVR_ATmega328__))
#define UART_IS_UART		0
#define UART_IS_USART		0
#define _SIG_UART_RECV		SIG_UART_RECV
#define _SIG_UART_DATA		SIG_UART_DATA
#define _UDR			UDR0
#define _UBRR			UBRR0
#elif (defined (__AVR_ATmega8__) || defined (__AVR_ATmega16__) || defined (__AVR_ATmega32__))
#define UART_IS_UART		1
#define UART_IS_USART		0
#define _SIG_UART_RECV		SIG_UART_RECV
#define _SIG_UART_DATA		SIG_UART_DATA
#define _UDR			UDR
#define _UBRR			UBRR
#elif (defined (__AVR_ATmega48__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega168) || \
	defined (__AVR_ATmega64) || defined (__AVR_ATmega128) )
#define UART_IS_USART		1
#define UART_IS_UART		0
#define _SIG_UART_RECV		SIG_USART_RECV
#define _SIG_UART_DATA		SIG_USART_DATA
#define _UDR			UDR0
#define _UBRR			UBRR0
#else
# warning "CPU type not supported by <uart.h>"
#endif

void uart_init(void);
unsigned char uart_poll(void);
void uart_flush(void);
char uart_getc(void);
void uart_putc(char c);

#endif
