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
#include <user_interface/menuSystem.h>
#include <user_interface/uiLocalisation.h>
#include <user_interface/uiUtilities.h>
#include <settings.h>
#include <usb_com.h>
#include <tnc/kissTNC.h>

// USB TX read/write positions and count
volatile uint16_t tncUSBComSendBufWritePosition = 0;
volatile uint16_t tncUSBComSendBufReadPosition = 0;
volatile uint16_t tncUSBComSendBufCount = 0;

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);
static void tncInitialize(void);
static void tncExit(void);
static void enqueueUSBData(volatile uint8_t *data, uint8_t length);
static void processUSBDataQueue(void);
static bool handleTNCRequest(void);

menuStatus_t menuTNCMode(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		tncInitialize();
		gMenusCurrentItemIndex = 5000;
	}
	else
	{
		if (ev->hasEvent)
		{
			handleEvent(ev);
		}
	}

	updateScreen();

	//Handle TNC RX Frames
	if(kiss_rx_frame_ready)
	{
		enqueueUSBData(kiss_frame_rx_buffer, kiss_frame_rx_buffer_idx);
		flushKISSRxFrameBuffer();
	}

	//Handle USB UART Data
	processUSBDataQueue();
	if (com_request == 1)
	{
		if (handleTNCRequest())
		{
			//Request was able to be handled
			com_request = 0;
		} else {
			//Request was not able to be handled for some reason...
			//TODO: Review this logic and do something about it
			com_request = 0;
		}
	}

	return MENU_STATUS_SUCCESS;
}

static void updateScreen(void)
{
	ucClearBuf();
	menuDisplayTitle(currentLanguage->tnc_mode);

	//Update Stats Strings
	char textBuffer[18];
	sprintf(textBuffer,"TX:%d/%d",kiss_stats_frames_tx,kiss_stats_bytes_tx);
	ucPrintCentered(1 * 8 + 16, textBuffer, FONT_SIZE_1);
	sprintf(textBuffer,"RX:%d/%d",kiss_stats_frames_rx,kiss_stats_bytes_rx);
	ucPrintCentered(2 * 8 + 16, textBuffer, FONT_SIZE_1);
	sprintf(textBuffer,"FR_ER:%d",kiss_error_counter_frame);
	ucPrintCentered(3 * 8 + 16, textBuffer, FONT_SIZE_1);
	sprintf(textBuffer,"OF_ER:%d",kiss_error_counter_buffer_overflow);
	ucPrintCentered(4 * 8 + 16, textBuffer, FONT_SIZE_1);

	ucRender();
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{
		tncExit();
	}
}

static void tncInitialize(void)
{
	enableKISSTNC();
}

static void tncExit(void)
{
	disableKISSTNC();
	settingsUsbMode = USB_MODE_CPS;
	menuHotspotRestoreSettings();
	menuSystemPopAllAndDisplayRootMenu();
}

// Queue system is a single byte header containing the length of the item, followed by the data
// if the block won't fit in the space between the current write location and the end of the buffer,
// a zero is written to the length for that block and the data and its length byte is put at the beginning of the buffer
static void enqueueUSBData(volatile uint8_t *data, uint8_t length)
{
	if ((tncUSBComSendBufWritePosition + (length + 1)) > (COM_BUFFER_SIZE - 1))
	{
		usbComSendBuf[tncUSBComSendBufWritePosition] = 0xFF; // flag that the data block won't fit and will be put at the start of the buffer
		tncUSBComSendBufWritePosition = 0;
	}

	usbComSendBuf[tncUSBComSendBufWritePosition] = length;
	for(int i=0;i<length;i++)
	{
		usbComSendBuf[tncUSBComSendBufWritePosition + i + 1] = data[i];
	}
	tncUSBComSendBufWritePosition += (length + 1);
	tncUSBComSendBufCount++;

	if (length <= 1)
	{
		// Blah
	}

}

static void processUSBDataQueue(void)
{
	if (tncUSBComSendBufCount > 0)
	{
		if (usbComSendBuf[tncUSBComSendBufReadPosition] == 0xFF) // End marker
		{
			usbComSendBuf[tncUSBComSendBufReadPosition] = 0;
			tncUSBComSendBufReadPosition = 0;
		}

		uint8_t len = usbComSendBuf[tncUSBComSendBufReadPosition] + 1;

		usb_status_t status = USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, &usbComSendBuf[tncUSBComSendBufReadPosition + 1], usbComSendBuf[tncUSBComSendBufReadPosition]);

		if (status == kStatus_USB_Success)
		{
			tncUSBComSendBufReadPosition += len;

			if (tncUSBComSendBufReadPosition >= (COM_BUFFER_SIZE - 1)) // reaching the end of the buffer
			{
				tncUSBComSendBufReadPosition = 0;
			}

			tncUSBComSendBufCount--;
		}
		else
		{
			// USB Send Fail
		}
	}
}

static bool handleTNCRequest(void)
{
	if(com_requestbuffer[0] == KISS_FEND && com_requestbuffer[1] == KISS_RETURN)
	{
		//TNC Exit command received
		tncExit();
		return true;
	}else{
		//Enqueue KISS Frame Data
		return sendKISSTxFrameDataPacket(com_requestbuffer,COM_REQUESTBUFFER_SIZE);
	}
	com_requestbuffer[0U] = 0xFFU; //Invalidate
}
