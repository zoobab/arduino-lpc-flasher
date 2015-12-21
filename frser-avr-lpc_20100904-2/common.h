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
 * \file common.h
 * \brief Common macros and definitions for AVR
 * 
 * Provides a range of convenience macros and compile-time
 * switchable debug macros.
 * 
 * \author	(C) 2007-2008 Mike Stirling 
 * 			($Author$)
 * \version	$Revision$
 * \date	$Date$
 */

#ifndef COMMON_H_
#define COMMON_H_

/*
 * Define standard types
 */

//! Defines a boolean type
typedef enum {
	False = 0,
	True
} bool_t;

#ifndef TRUE
//! Alternative to "True"
#define TRUE			(True)
#endif
#ifndef FALSE
//! Alternative to "False"
#define FALSE			(False)
#endif

#ifndef NULL
//! A NULL pointer value
#define NULL			(void*)(0)
#endif

// Define debugging macros.  These require extra include files
// which the following will pull in.
#ifdef DEBUG
#include <stdio.h>
#include <avr/pgmspace.h>
#include "debug.h"
#include "ansi.h"
#endif

#if DEBUG > 0
//! Outputs red debug messages when debug level is 1 or more
#define ERROR(a,...)			{ fprintf_P(stderr,PSTR(ATTR_RESET FG_RED __FILE__ "(%d): " ATTR_RESET a),__LINE__,##__VA_ARGS__); }
#else
#define ERROR(...)
#endif

#if DEBUG > 1
//! Outputs yellow debug messages when debug level is 2 or more
#define INFO(a,...)				{ fprintf_P(stderr,PSTR(ATTR_RESET FG_YELLOW __FILE__ "(%d): " ATTR_RESET a),__LINE__,##__VA_ARGS__); }
#else
#define INFO(...)
#endif

#if DEBUG > 2
//! Outputs green debug messages when debug level is 3 or more
#define TRACE(a,...)			{ fprintf_P(stderr,PSTR(ATTR_RESET FG_GREEN __FILE__ "(%d): " ATTR_RESET a),__LINE__,##__VA_ARGS__); }
#else
#define TRACE(...)
#endif

#if DEBUG > 3
//! Intended for use in ISRs, outputs cyan debug messages when debug level is 4 or more
#define ISR_TRACE(a,...)		{ fprintf_P(stderr,PSTR(ATTR_RESET FG_CYAN __FILE__ "(%d): " ATTR_RESET a),__LINE__,##__VA_ARGS__); }
#else
#define ISR_TRACE(...)
#endif

/*
 * Byte manipulation macros
 */

//! Select the low byte of a 16-bit word
#define UINT16_LOW(a)			(a & 0xff)

//! Select the high byte of a 16-bit word
#define UINT16_HIGH(a)			(a >> 8)

/*
 * Bit manipulation macros
 */

//! Set bits in a register
#define SET(reg, flags)			((reg) = (reg) | (flags))
//! Clear bits in a register
#define CLEAR(reg, flags)		((reg) &= ~(flags))

//! Determine if bits in a register are set
#define ISSET(reg, flags)		(((reg) & (flags)) == (flags))
//! Determine if bits in a register are clear
#define ISCLEAR(reg, flags)		(((reg) & (flags)) == 0)

//! Inserts a no-operation assembly instruction
#define nop()					asm volatile ("nop")

//! This global is provided by version.c, which should be rebuilt every time
extern const char *build_version;

//! Provide access to the global tick counter.  Should be defined in main.
uint32_t gettickcount(void);

#endif /*COMMON_H_*/
