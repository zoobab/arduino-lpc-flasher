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
 * \file lpc.c
 * \brief LPC bus emulation state machine
 * 
 * \author	(C) 2010 Mike Stirling 
 * 			($Author$)
 * \version	$Revision$
 * \date	$Date$
 */

#include <stdint.h>
#include <avr/io.h>

#include "board.h"
#include "common.h"

typedef enum {
	lpcReset = 0,
	lpcResetWait,
	lpcInit,
	lpcInitWait,
	lpcIdle,
	lpcStart,
	lpcCommand,
	lpcAddress,
	lpcDataOut,
	lpcTARtoInput,
	lpcSync,
	lpcDataIn,
	lpcTARtoOutput,
} lpc_state_t;

typedef struct {
	lpc_state_t		state;
	uint8_t			command;
	uint32_t		address;
	uint8_t			data;
	uint8_t			count;
	uint8_t			clock:1;
} lpc_t;
static lpc_t lpc;

#define LPC_MEMORY_READ		0x4
#define LPC_MEMORY_WRITE	0x6

#define LPC_RESET_COUNT		10
#define LPC_INIT_COUNT		10

void lpc_state_machine(void)
{	
	lpc.clock = !lpc.clock;
	if (lpc.clock) {
		LPC_PORT |= _BV(LPC_CLK);
	} else {
		LPC_PORT &= ~_BV(LPC_CLK);
	}
	
	// Data changes occur on the falling clock edges
	if (lpc.clock)
		return;
	
	switch (lpc.state) {
	case lpcReset:
		TRACE("LPC reset started\n");
		LPC_RESET_PORT &= ~_BV(LPC_nRESET);
		LPC_DDR |= 0xf;
		LPC_PORT &= ~0xff;
		LPC_PORT |= _BV(LPC_nLFRAM);
		lpc.count = LPC_RESET_COUNT;
		lpc.state = lpcResetWait;
		break;
	case lpcResetWait:
		lpc.count--;
		if (lpc.count == 0)
			lpc.state = lpcInit;
		break;
	case lpcInit:
		LPC_RESET_PORT |= _BV(LPC_nRESET);
		lpc.count = LPC_INIT_COUNT;
		lpc.state = lpcInitWait;
		break;
	case lpcInitWait:
		lpc.count--;
		if (lpc.count == 0) {
			TRACE("LPC reset complete\n");
			lpc.state = lpcIdle;
		}
			
	case lpcIdle:
		LPC_PORT &= ~0xf;
		LPC_PORT |= _BV(LPC_nLFRAM);
		break;
	case lpcStart:
		TRACE("LPC transaction starting\n");
		LPC_PORT &= ~0xf;
		LPC_PORT &= ~_BV(LPC_nLFRAM);
		lpc.state = lpcCommand;
		break;
	case lpcCommand:
		//TRACE("Command = %x\n",lpc.command);
		LPC_PORT = (LPC_PORT & ~0xf) | (lpc.command & 0xf);
		LPC_PORT |= _BV(LPC_nLFRAM);
		lpc.state = lpcAddress;
		lpc.count = 8; // address clocked out over 8 cycles
		break;
	case lpcAddress:
		//TRACE("Address byte = %x\n",lpc.address >> 28);
		LPC_PORT = (LPC_PORT & ~0xf) | (lpc.address >> 28);
		lpc.address <<= 4;
		lpc.count--;
		if (lpc.count == 0) {
			lpc.state = (lpc.command == LPC_MEMORY_READ) ? lpcTARtoInput : lpcDataOut;
			lpc.count = 2;
		}
		break;
	case lpcDataOut:
		LPC_PORT = (LPC_PORT & ~0xf) | (lpc.data & 0xf);
		lpc.data >>= 4;
		//TRACE("Data out = %x\n",LPC_PORT & 0xf);
		lpc.count--;
		if (lpc.count == 0) {
			lpc.state = lpcTARtoInput;
			lpc.count = 2;
		}
		break;
	case lpcTARtoInput:
		LPC_PORT |= 0xf;
		lpc.count--;
		if (lpc.count == 1)
			LPC_DDR &= ~0xf; // tri-state data bus
		if (lpc.count == 0)
			lpc.state = lpcSync;
		break;
	case lpcSync:
		if ((LPC_PIN & 0xf) == 5) {
			TRACE("Device inserted wait state\n");
			break;
		}
		if ((LPC_PIN & 0xf) != 0) {
			ERROR("Bad sync word received 0x%x != 0\n",LPC_PIN & 0xf);
			lpc.state = lpcTARtoOutput; // must restore port to output mode
			lpc.count = 2;
			break;
		}
		lpc.state = (lpc.command == LPC_MEMORY_READ) ? lpcDataIn : lpcTARtoOutput;
		lpc.count = 2; // 2 nibbles
		break;
	case lpcDataIn:
		//TRACE("Data in = %x\n",LPC_PIN & 0xf);
		lpc.count--;
		if (lpc.count == 1)
			lpc.data = LPC_PIN & 0xf;
		if (lpc.count == 0) {
			lpc.data |= (LPC_PIN & 0xf) << 4;
			lpc.state = lpcTARtoOutput;
			lpc.count = 2;
		}
		break;
	case lpcTARtoOutput:
		LPC_PORT |= 0xf;
		lpc.count--;
		if (lpc.count == 1)
			LPC_DDR |= 0xf; // bus to output
		if (lpc.count == 0)
			lpc.state = lpcIdle;
		break;
	default:
		ERROR("Bad state\n");
	}
	
	TRACE("nRESET = %d nLFRAM=%d LAD=%X\n",(LPC_RESET_PORT & _BV(LPC_nRESET)) ?  1 : 0,(LPC_PORT & _BV(LPC_nLFRAM)) ? 1 : 0,LPC_PIN & 0xf);
}

void lpc_reset(void)
{
	lpc.state = lpcReset;
	while (lpc.state != lpcIdle)
		lpc_state_machine();
}

uint8_t lpc_read(uint32_t address)
{
	lpc.command = LPC_MEMORY_READ;
	lpc.address = address;
	lpc.state = lpcStart;
	while (lpc.state != lpcIdle)
		lpc_state_machine();
	return lpc.data;
}

void lpc_write(uint32_t address,uint8_t data)
{
	lpc.command = LPC_MEMORY_WRITE;
	lpc.address = address;
	lpc.data = data;
	lpc.state = lpcStart;
	while (lpc.state != lpcIdle)
		lpc_state_machine();	
}
