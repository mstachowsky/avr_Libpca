/* Copyright (C) 
 * 2013 - Tomasz Wisniewski
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */


/**
 * @file slip.c
 *
 * @brief Implementation of SLIP Send/Recv routines
 *
 */

#include "slip.h"
#include <util/crc16.h>
#include <string.h>

unsigned char slip_recv(unsigned char *a_buff, unsigned char a_buflen) {
	
	unsigned char c = 0x00;
	unsigned int recv = 0x00;
	unsigned char mode = 0x00;

	// collect a full slip packet
	while (1) {
		if (!SLIP_CHAR_RECV(&c)) {
			continue;
		}

		// no escape character detected
		if (!mode) {
			switch(c) {
				case SLIP_END:
					if (recv) return recv;
					break;

				case SLIP_ESC:
					// escape character detected set mode variable
					mode = 0x01;
					break;

				default:
					a_buff[recv++] = c;
					break;

			} // switch
		}
		else {
			// translate escaped character to it's original value
			a_buff[recv++] = 
				(c == SLIP_ESC_END ? SLIP_END : (c == SLIP_ESC_ESC ? SLIP_ESC : c));
			mode = 0;
		}

		// if buffer is full abort further reception
		// this may cause problems - one should always provide big enough buffer for the
		// whole SLIP frame to fit
		if (recv >= a_buflen) {
			return recv;
		}
	} // while

	return 0;
}


unsigned char slip_send(unsigned char *a_buff, unsigned char a_buflen) {

	unsigned char send = a_buflen;

	// flush buffers at the receiving side
	SLIP_CHAR_SEND(SLIP_END);

	// transmit the data
	while (a_buflen) {
		switch (*a_buff) {
			// escape END character
			case SLIP_END:
				SLIP_CHAR_SEND(SLIP_ESC);
				SLIP_CHAR_SEND(SLIP_ESC_END);
				break;

			// escape ESC character
			case SLIP_ESC:
				SLIP_CHAR_SEND(SLIP_ESC);
				SLIP_CHAR_SEND(SLIP_ESC_ESC);
				break;

			default:
				SLIP_CHAR_SEND(*a_buff);
				break;
		} // switch

		a_buff++;
		a_buflen--;
	}

	// mark the transmission end
	SLIP_CHAR_SEND(SLIP_END);
	return send;
}


unsigned char slip_verify_crc16(unsigned char *a_buff, unsigned char a_buflen, unsigned char a_crcpos) {
	
	uint16_t crc_recv = 0x00;
	uint16_t crc_calcd = 0x00;

	if ((a_crcpos + 2) > a_buflen || !a_buflen) {
		return 0;
	}

	// the transmitter calculated the CRC the same way
	// marking the memory being a CRC placeholder with zeroes
	// and then copying the CRC inside
	memcpy(&crc_recv, &a_buff[a_crcpos], 2);
	memset(&a_buff[a_crcpos], 0x00, 2);

	while (a_buflen) {
		crc_calcd = _crc16_update(crc_calcd, *a_buff);
		a_buff++;
		a_buflen--;
	}

	return (crc_calcd == crc_recv ? crc_calcd : 0);
}


unsigned char slip_append_crc16(unsigned char *a_buff, unsigned char a_datalen) {

	uint16_t crc_calcd = 0x00;
	
	memset(&a_buff[a_datalen], 0x00, 2);
	for (uint8_t i = 0; i<(a_datalen + 2); i++) {
		crc_calcd = _crc16_update(crc_calcd, a_buff[i]);
	}

	memcpy(&a_buff[a_datalen], &crc_calcd, 2);
	return a_datalen + 2;
}