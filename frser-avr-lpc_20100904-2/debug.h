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
 * \file debug.h
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

#ifndef DEBUG_H_
#define DEBUG_H_

#ifndef F_CPU
# warning "F_CPU not defined for <debug.h>"
# define F_CPU 1000000UL
#endif

/*
 * Module configuration
 */

/// Port number (where more than one available)
#define DEBUG_PORT			0

/// Desired baud rate
#define DEBUG_BAUD			115200

/// Number of data bits (5,6,7 or 8)
#define DEBUG_DATABITS		8

/// Number of stop bits (1 or 2)
#define DEBUG_STOPBITS		1

/// Parity (0 = none, 1 = odd, 2 = even)
#define DEBUG_PARITY		0

/// If the UART is to operate in high-speed mode
#define DEBUG_HIGHSPEED		1

/// If the UART is to operate in synchronous mode
#define DEBUG_SYNC			0

/// If the UART is to bind to stdio
#ifdef DEBUG
#define DEBUG_IS_STDIO		1
#else
#define DEBUG_IS_STDIO		0
#endif

/*
 * Internal macros
 */

/// Divider value for baud rate calculation
#if DEBUG_SYNC
#define DEBUG_DIV			2
#elif DEBUG_HIGHSPEED
#define DEBUG_DIV			8
#else
#define DEBUG_DIV			16
#endif

/// Calculated baud rate constant
#define DEBUG_BAUDCONST		(F_CPU/DEBUG_DIV/DEBUG_BAUD-1)

/*
 * Definitions
 */

/// Initialise the debug serial port and bind it to stdio if required
void debug_init(void);

/// Returns a single character from the debug serial port, blocking until
/// one is available.
/// \return Received character
char debug_getc(void);

/// Sends a single character out of the debug serial port, blocking until
/// the transmission can commence.
/// \param c Character to send
void debug_putc(char c);

#endif /*DEBUG_H_*/
