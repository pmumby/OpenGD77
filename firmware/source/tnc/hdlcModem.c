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

#include <tnc/kissTNC.h>
#include <tnc/hdlcModem.h>
#include <settings.h>
#include <trx.h>

volatile uint8_t hdlc_frame_rx_buffer[HDLC_FRAME_BUFFER_SIZE];
volatile uint8_t hdlc_frame_tx_buffer[HDLC_FRAME_BUFFER_SIZE];
int hdlc_frame_rx_buffer_idx;
int hdlc_frame_tx_buffer_idx;
bool hdlc_rx_frame_ready;
bool hdlc_tx_frame_ready;

/*
static void enableTransmission(void);
static void disableTransmission(void);
*/

void handleHDLCTXFrame(void)
{
	//Are we in KISS Frame Loopback mode (HDLC Bypass)?
	if(kiss_config_loopback==0x01)
	{
		//Yup, so let's just loopback!
		//First confirm the RX buffer able to receive a frame, otherwise skip and wait...
		if(!hdlc_rx_frame_ready){
			//Ok, it's clear, let's go ahead and copy the tx frame buffer straight into the rx frame buffer as-is!
			for(int i=0;i<HDLC_FRAME_BUFFER_SIZE;i++)
			{
				hdlc_frame_rx_buffer[i] = hdlc_frame_tx_buffer[i];
			}
			hdlc_frame_rx_buffer_idx = hdlc_frame_tx_buffer_idx;
			//Notify of new rx frame:
			hdlc_rx_frame_ready = true;
			//And flush the TX buffer:
			flushHDLCTxFrameBuffer();
		}
	}else{
		//Nope, continue as usual...
		//TODO!

		//For now we'll just flush it! lol
		flushHDLCTxFrameBuffer();
	}
	kiss_stats_frames_tx++;
}

void tickHDLCModem(void)
{
	//TODO: Put proper modem logic in here. For now just loopback to test

	//Do we have a pending frame to transmit?
	if(hdlc_tx_frame_ready)
	{
		handleHDLCTXFrame();
	}
}

/*
//Internal... Turns on TX on the radio
static void enableTransmission(void)
{
	GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
	GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 1);
	txstopdelay = 0;
	trx_setTX();
}

//Internal... Turns off TX on the radio
static void disableTransmission(void)
{
	GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 0);
	// Need to wrap this in Task Critical to avoid bus contention on the I2C bus.
	taskENTER_CRITICAL();
	trxActivateRx();
	taskEXIT_CRITICAL();
}
*/

//Flushes the RX buffer from the TNC
//Call this from the "host" to indicate you have read the buffer contents
//Will flush the buffer and remove the data ready flag
//Allows next frame to be received
void flushHDLCRxFrameBuffer(void)
{
	hdlc_frame_tx_buffer_idx = 0;
	hdlc_rx_frame_ready = false;
}

//Flushes the TX buffer from the TNC
//This is primarily used as an internal function as part of the packet handling code
//But can also be used by the host in the event for some reason known malformed data has made it into the buffer.
//Essentially resets the buffer and logic.
void flushHDLCTxFrameBuffer(void)
{
	hdlc_frame_tx_buffer_idx = 0;
	hdlc_tx_frame_ready = false;
}

