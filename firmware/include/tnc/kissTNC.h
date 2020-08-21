/*
 * Copyright (C)2020 Paul Mumby ZL1GW
 * Contributed to OpenGD77 Firmware Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _FW_KISS_TNC_H_
#define _FW_KISS_TNC_H_
#include "main.h"

//KISS Standard allows for 1024 Byte Frames, but since technically each byte could be doubled due to FESC, need 2048 + 3 (header/footer)
#define KISS_FRAME_BUFFER_SIZE (2048+3)

// Counters & Flags
extern int kiss_error_counter_buffer_overflow;
extern int kiss_error_counter_frame;
extern bool kiss_frame_start_skipped;
extern int kiss_stats_frames_rx;
extern int kiss_stats_frames_tx;
extern int kiss_stats_bytes_rx;
extern int kiss_stats_bytes_tx;


// TX & RX Buffers and errors
extern volatile uint8_t kiss_frame_rx_buffer[KISS_FRAME_BUFFER_SIZE];
extern int kiss_frame_rx_buffer_idx;
extern bool kiss_rx_frame_ready;

// KISS Standard TNC Config values
extern uint8_t kiss_config_persistence;
extern uint8_t kiss_config_slottime;
extern uint8_t kiss_config_txdelay;
extern uint8_t kiss_config_txtail;
extern bool kiss_config_fullduplex;

// Additional Config unique to this implementation (using SETHW command)
// Using 2 bytes, one to indicate the config address and the other to update the value
//					address		default		details
// loopback			0x00		0x00		0x00 is disabled
//											0x01 is loopback of kiss frames (bypassing HDLC modem, but handled in HDLC tick function)
//											0x02 is loopback through the HDLC modem
// tone				0x01		0x00		0x00 is disabled, any other value is isochronous test tone in 100hz steps
// txinhibit		0x02		0x00		0x00 is "OFF" meaning tx is enabled
//											0x01 is "ON" meaning tx is inhibited (for testing purposes etc)
// txforce			0x03		0x00		0x00 is "OFF"
//											0x01 is "ON" meaning TX is forced active (PTT held down) until disabled
// audiofeedback	0x04		0x00		0x00 is "OFF" meaning tx is enabled
//											0x01 is "ON" meaning generated audio is looped to the speaker for feedback
typedef enum
{
	KISS_CFG_ADDR_LOOPBACK		= 0x00,
	KISS_CFG_ADDR_TONE			= 0x01,
	KISS_CFG_ADDR_TXINHIBIT		= 0x02,
	KISS_CFG_ADDR_TXFORCE		= 0x03,
	KISS_CFG_ADDR_AUDIOFEEDBACK	= 0x04
} KISS_CONFIG_ADDR;

extern uint8_t kiss_config_loopback;
extern uint8_t kiss_config_tone;
extern uint8_t kiss_config_txinhibit;
extern uint8_t kiss_config_txforce;
extern uint8_t kiss_config_audiofeedback;

typedef enum
{
	//KISS Control Command Types
	KISS_DF      	= 0x00,
	KISS_TXDELAY	= 0x01,
	KISS_P			= 0x02,
	KISS_SLOTTIME	= 0x03,
	KISS_TXTAIL		= 0x04,
	KISS_FULLDUPLEX	= 0x05,
	KISS_SETHW		= 0x06,
	KISS_RETURN		= 0xFF,
	//KISS Frame Format
	KISS_FEND		= 0xC0,
	KISS_FESC		= 0xDB,
	KISS_TFEND		= 0xDC,
	KISS_TFESC		= 0xDD
} KISS_PROTOCOL;

extern bool kissTNCActive;

void enableKISSTNC(void);
void disableKISSTNC(void);
void tickKISSTNC(void);
bool sendKISSTxFrameDataPacket(volatile uint8_t *dataBuf,int bytes);
void flushKISSRxFrameBuffer(void);
void flushKISSTxFrameBuffer(void);

#endif
