/*
 * This file is part of the frser-avr project.
 *
 * Copyright (C) 2009 Urja Rannikko <urjaman@gmail.com>
 * Modified 2010 Mike Stirling
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
 */

#include <avr/io.h>
#include <util/delay.h>

#include "board.h"
#include "common.h"
#include "lpc.h"
#include "uart.h"

/* Flashrom serial interface AVR implementation */


const char pgmname[16] = "ATMega88 LPC   ";

/* The length of the operation buffer */
#define S_OPBUFLEN 	200
/* The maximum static length of parameters (read n) */
#define S_MAXLEN 	0x06

static unsigned char opbuf[S_OPBUFLEN];
static unsigned int opbuf_bytes = 0;

#define S_ACK 0x06
#define S_NAK 0x15
#define S_CMD_NOP               0x00            /* No operation                                 */
#define S_CMD_Q_IFACE           0x01            /* Query interface version                      */
#define S_CMD_Q_CMDMAP          0x02            /* Query supported commands bitmap              */
#define S_CMD_Q_PGMNAME         0x03            /* Query programmer name                        */
#define S_CMD_Q_SERBUF          0x04            /* Query Serial Buffer Size                     */
#define S_CMD_Q_BUSTYPE         0x05            /* Query supported bustypes                     */
#define S_CMD_Q_CHIPSIZE        0x06            /* Query supported chipsize (2^n format)        */
#define S_CMD_Q_OPBUF           0x07            /* Query operation buffer size                  */
#define S_CMD_Q_WRNMAXLEN       0x08            /* Query Write to opbuf: Write-N maximum lenght */
#define S_CMD_R_BYTE            0x09            /* Read a single byte                           */
#define S_CMD_R_NBYTES          0x0A            /* Read n bytes                                 */
#define S_CMD_O_INIT            0x0B            /* Initialize operation buffer                  */
#define S_CMD_O_WRITEB          0x0C            /* Write opbuf: Write byte with address         */
#define S_CMD_O_WRITEN          0x0D            /* Write to opbuf: Write-N                      */
#define S_CMD_O_DELAY           0x0E            /* Write opbuf: udelay                          */
#define S_CMD_O_EXEC            0x0F            /* Execute operation buffer                     */
#define S_CMD_SYNCNOP           0x10            /* Special no-operation that returns NAK+ACK    */
#define S_CMD_Q_RDNMAXLEN       0x11            /* Query read-n maximum length                  */
#define S_CMD_S_BUSTYPE         0x12            /* Set used bustype(s).     */

/* The biggest valid command value */
#define S_MAXCMD 				S_CMD_Q_RDNMAXLEN


#define OPBUF_WRITEOP 0x00
#define OPBUF_DELAYOP 0x01

#define RECEIVE()		uart_getc()
#define SEND(a)			uart_putc(a)
#define UART_BUFLEN		96

static void udelay(unsigned long int usecs) {
	if (usecs < 16) {
		unsigned char i=usecs;
		do { _delay_us(1); } while(--i);
		return;
	}
	usecs >>= 4; // div 16;
	do { _delay_us(16); } while(--usecs);
	return;
}

static unsigned char opbuf_addbyte(unsigned char c) {
	if (opbuf_bytes == S_OPBUFLEN) return 1;
	opbuf[opbuf_bytes++] = c;
	return 0;
}

static unsigned long int buf2u24(unsigned char*buf) {
	unsigned long int u24;
	u24  = (((unsigned long int)buf[0])<< 0);
	u24 |= (((unsigned long int)buf[1])<< 8);
	u24 |= (((unsigned long int)buf[2])<<16);
	return u24;
}

static void do_cmd_readbyte(unsigned char* parbuf) {
	unsigned char c;
	unsigned long int addr;
	addr = buf2u24(parbuf);
	c = lpc_read(0xfff80000 | addr);
	SEND(S_ACK);
	SEND(c);
}

static void do_cmd_readnbytes(unsigned char* parbuf) {
	unsigned long int i,addr,n;
	addr = buf2u24(parbuf);
	n = buf2u24(parbuf+3);
	SEND(S_ACK);
	for(i=addr;i<(addr+n);i++) {
		unsigned char c;
		c = lpc_read(0xfff80000 | i);
		SEND(c);
	}
}

static void do_cmd_opbuf_writeb(unsigned char* parbuf) {
	if (opbuf_addbyte(OPBUF_WRITEOP)) goto nakret;
	if (opbuf_addbyte(1)) goto nakret;
	if (opbuf_addbyte(parbuf[0])) goto nakret;
	if (opbuf_addbyte(parbuf[1])) goto nakret;
	if (opbuf_addbyte(parbuf[2])) goto nakret;
	if (opbuf_addbyte(parbuf[3])) goto nakret;
	SEND(S_ACK);
	return;
nakret:
	SEND(S_NAK);
	return;
}




static void do_cmd_opbuf_delay(unsigned char* parbuf) {
	if (opbuf_addbyte(OPBUF_DELAYOP)) goto nakret;
	if (opbuf_addbyte(parbuf[0])) goto nakret;
	if (opbuf_addbyte(parbuf[1])) goto nakret;
	if (opbuf_addbyte(parbuf[2])) goto nakret;
	if (opbuf_addbyte(parbuf[3])) goto nakret;
	SEND(S_ACK);
	return;
nakret:
	SEND(S_NAK);
	return;
	}

static void do_cmd_opbuf_writen(void) {
	unsigned char len;
	unsigned char plen = 3;
	unsigned char i;
	len = RECEIVE();
	RECEIVE();
	RECEIVE();
	if (opbuf_addbyte(OPBUF_WRITEOP)) goto nakret;
	if (opbuf_addbyte(len)) goto nakret;
	plen--; if (opbuf_addbyte(RECEIVE())) goto nakret;
	plen--; if (opbuf_addbyte(RECEIVE())) goto nakret;
	plen--; if (opbuf_addbyte(RECEIVE())) goto nakret;
	for(;;) {
		if (opbuf_addbyte(RECEIVE())) goto nakret;
		len--;
		if (len == 0) break;
	}
	SEND(S_ACK);
	return;
	
nakret:
	for(i=0;i<plen;i++) RECEIVE();
	for(i=0;i<len;i++) RECEIVE();
	SEND(S_NAK);
	return;
	
}
	
static void do_cmd_opbuf_exec(void) {
	unsigned int readptr;
	for(readptr=0;readptr<opbuf_bytes;) {
		unsigned char op;
		op = opbuf[readptr++];
		if (readptr >= opbuf_bytes) goto nakret;
		if (op == OPBUF_WRITEOP) {
			unsigned long int addr;
			unsigned char len,i;
			len = opbuf[readptr++];
			if (readptr >= opbuf_bytes) goto nakret;
			addr = buf2u24(opbuf+readptr);
			readptr += 3;
			if (readptr >= opbuf_bytes) goto nakret;
			for(i=0;;) {
				unsigned char c;
				c = opbuf[readptr++];
				if (readptr > opbuf_bytes) goto nakret;
				lpc_write(0xfff80000 | addr,c);
				addr++;
				i++;
				if (i==len) break;
			}
			continue;
		}
		if (op == OPBUF_DELAYOP) {
			unsigned long int usecs;
			usecs  = (((unsigned long int)(opbuf[readptr++])) << 0);
			usecs |= (((unsigned long int)(opbuf[readptr++])) << 8);
			usecs |= (((unsigned long int)(opbuf[readptr++])) << 16);
			usecs |= (((unsigned long int)(opbuf[readptr++])) << 24);
			if (readptr > opbuf_bytes) goto nakret;
			udelay(usecs);
			continue;
		}
		goto nakret;
	}
	opbuf_bytes = 0;
	SEND(S_ACK);
	return;
nakret:
	opbuf_bytes = 0;
	SEND(S_NAK);
	return;
}
	
const unsigned char op2len[S_MAXCMD+1] = { /* A table to get  parameter length from opcode if possible (if not 0) */
		0x00, 0x00, 0x00,	/* NOP, iface, bitmap */
		0x00, 0x00, 0x00,	/* progname, serbufsize, bustypes */
		0x00, 0x00, 0x00,	/* chipsize, opbufsz, query-n maxlen */
		0x03, 0x06, 0x00,	/* read byte, read n, init opbuf */
		0x04, 0x00, 0x04,	/* write byte, write n, write delay */
		0x00, 0x00, 0x00,	/* Exec opbuf, SYNCNOP, query read maxlen */
	};
	

void frser_main(void) {
	for(;;) {
		unsigned char parbuf[S_MAXLEN]; /* Parameter buffer */
		unsigned char len;
		unsigned char op;
		unsigned char i;
		op = RECEIVE();
		if (op > S_MAXCMD) {
			/* This is a pretty futile case as in that we shouldnt get
			these commands at all with the new supported cmd bitmap system*/
			SEND(S_NAK);
			continue;
		}
		len = op2len[op];
		
		LED_PORT ^= _BV(LED0);
		
		for (i=0;i<len;i++) parbuf[i] = RECEIVE();
		switch (op) {
			case S_CMD_NOP:
				SEND(S_ACK);
				break;
			case S_CMD_SYNCNOP:
				SEND(S_NAK);
				SEND(S_ACK);
				break;
			case S_CMD_Q_IFACE:
				SEND(S_ACK);
				SEND(0x01);
				SEND(0x00);
				break;
			case S_CMD_Q_CMDMAP: /* a simple map with 2 bytes 0xFF, rest 0 */
				SEND(S_ACK);
				SEND(0xFF);
				SEND(0xFF);
				SEND(0x03);
				for(i=0;i<29;i++) SEND(0x00);
				break;
			case S_CMD_Q_PGMNAME:
				SEND(S_ACK);
				for(i=0;i<16;i++) SEND(pgmname[i]);
				break;
			case S_CMD_Q_SERBUF:
				SEND(S_ACK);
				SEND(UART_BUFLEN&0xFF);
				SEND((UART_BUFLEN>>8)&0xFF);
				break;
			case S_CMD_Q_BUSTYPE:
				SEND(S_ACK);
				SEND(0x02); /* Only LPC */
				break;
			case S_CMD_Q_CHIPSIZE: /* 512k */
				SEND(S_ACK);
				SEND(19);
				break;
			case S_CMD_Q_OPBUF:
				SEND(S_ACK);
				SEND(S_OPBUFLEN&0xFF);
				SEND((S_OPBUFLEN>>8)&0xFF);
				break;
			case S_CMD_Q_WRNMAXLEN: /* 256 bytes */
				SEND(S_ACK);
				SEND(0);
				SEND(1);
				SEND(0);
				break;
			case S_CMD_R_BYTE:
				do_cmd_readbyte(parbuf);
				break;
			case S_CMD_R_NBYTES:
				do_cmd_readnbytes(parbuf);
				break;
			case S_CMD_O_INIT:
				opbuf_bytes = 0;
				SEND(S_ACK);
				break;
			case S_CMD_O_WRITEB:
				do_cmd_opbuf_writeb(parbuf);
				break;
			case S_CMD_O_WRITEN:
				do_cmd_opbuf_writen();
				break;
			case S_CMD_O_DELAY:
				do_cmd_opbuf_delay(parbuf);
				break;
			case S_CMD_O_EXEC:
				do_cmd_opbuf_exec();
				break;
			case S_CMD_Q_RDNMAXLEN: /* unlimited */
				SEND(S_ACK);
				SEND(0);
				SEND(0);
				SEND(0);
				break;
		}
	}
}