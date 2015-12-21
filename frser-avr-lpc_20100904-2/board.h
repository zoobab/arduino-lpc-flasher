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
 * \file board.h
 * \brief Hardware definitions for LPC flash programmer on STK500 (ATMEGA88)
 * 
 * \author	(C) 2010 Mike Stirling 
 * 			($Author$)
 * \version	$Revision$
 * \date	$Date$
 */

#ifndef BOARD_H_
#define BOARD_H_

#define LED_PORT			PORTB
#define LED0				PB0
#define LED1				PB1

#define LPC_PORT			PORTC
#define LPC_DDR				DDRC
#define LPC_PIN				PINC
#define LPC_nLFRAM			PC4
#define LPC_CLK				PC5

#define LPC_RESET_PORT		PORTB
#define LPC_nRESET			PB2

/*
 * Output port macros
 */

//! Macro to turn an output on
#define OUTPUT_ON(a)			(OUTPUTS_PORT |= _BV(a))
//! Macro to turn an output off
#define OUTPUT_OFF(a)			(OUTPUTS_PORT &= ~_BV(a))

/*
 * SPI port IO definitions
 */

#define SS					PB2
#define MOSI				PB3
#define MISO				PB4
#define SCK					PB5

/*
 * UART port IO definitions
 */

#define TXD0				PD1
#define RXD0				PD0

/* 
 * Input definitions for each hardware port
 */

#define PORTA_INS			(0)
#define PORTB_INS			(0)
#define PORTC_INS			(0)
#define PORTD_INS			(_BV(RXD0))

/*
 * Output definitions for each hardware port
 */

#define PORTA_OUTS			(0)
#define PORTB_OUTS			(_BV(LED0) | _BV(LED1) | _BV(LPC_nRESET))
#define PORTC_OUTS			(0x0f | _BV(LPC_nLFRAM) | _BV(LPC_CLK))
#define PORTD_OUTS			(_BV(TXD0))

/*
 * Initial port values (to enable/disable pull-ups, define startup conditions, etc.)
 */

#define PORTA_VAL			(0)
#define PORTB_VAL			(_BV(LED0) | _BV(LED1))
#define PORTC_VAL			(0)
#define PORTD_VAL			(0)

/*
 * Initial DDR values - unused pins are set to outputs to save power
 */

#define DDRA_VAL			(0xff & ~PORTA_INS)
#define DDRB_VAL			(0xff & ~PORTB_INS)
#define DDRC_VAL			(0xff & ~PORTC_INS)
#define DDRD_VAL			(0xff & ~PORTD_INS)

#endif /*BOARD_H_*/
