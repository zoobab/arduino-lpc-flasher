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
 * \file main.c
 * \brief LPC flash programmer top-level
 * 
 * \author	(C) 2010 Mike Stirling 
 * 			($Author$)
 * \version	$Revision$
 * \date	$Date$
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>

#include "board.h"
#include "common.h"
#include "lpc.h"
#include "frser.h"
#include "uart.h"

int main(void)
{
	uint8_t	mcustatus = MCUSR;
	
	MCUSR = 0;
	
	// Set up port directions and load initial values/enable pull-ups
	PORTB = PORTB_VAL;
	PORTC = PORTC_VAL;
	PORTD = PORTD_VAL;
	DDRB = DDRB_VAL;
	DDRC = DDRC_VAL;
	DDRD = DDRD_VAL;
	
	// Enable interrupts
	sei();

	/* Bring up serial port */
	uart_init();
	uart_flush();
	_delay_ms(20);
	
	INFO("\n\n" BG_BLUE FG_YELLOW "  LPC Flash Programmer  " ATTR_RESET "\n\n");
	INFO("Build timestamp: %s\n",build_version);
	INFO("Debug level = %u\n",DEBUG);
	INFO("System clock = %lu Hz\n",F_CPU);
	INFO("Reset status = 0x%X\n",mcustatus);

	/* Start main loop */
	lpc_reset();
	INFO("Entering main loop\n");
	frser_main();

	while(1) {
	}
	
	return 0;
}
