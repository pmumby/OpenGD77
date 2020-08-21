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
#ifndef _FW_HDLC_MODEM_H_
#define _FW_HDLC_MODEM_H_
#include "main.h"

//KISS Standard allows for 1024 Byte Frames, with 64bytes extra in case of control characters and buffer misalignment
#define HDLC_FRAME_BUFFER_SIZE (1024+64)

// TX & RX Buffers and errors
extern volatile uint8_t hdlc_frame_rx_buffer[HDLC_FRAME_BUFFER_SIZE];
extern volatile uint8_t hdlc_frame_tx_buffer[HDLC_FRAME_BUFFER_SIZE];
extern int hdlc_frame_rx_buffer_idx;
extern int hdlc_frame_tx_buffer_idx;
extern bool hdlc_rx_frame_ready;
extern bool hdlc_tx_frame_ready;

void handleHDLCTXFrame(void);
void tickHDLCModem(void);
void flushHDLCRxFrameBuffer(void);
void flushHDLCTxFrameBuffer(void);

#endif
