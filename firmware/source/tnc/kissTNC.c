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

// Counters & Flags
int kiss_error_counter_buffer_overflow = 0;
int kiss_error_counter_frame = 0;
bool kiss_frame_start_skipped = false;
int kiss_stats_frames_rx = 0;
int kiss_stats_frames_tx = 0;
int kiss_stats_bytes_rx = 0;
int kiss_stats_bytes_tx = 0;

// TX & RX Buffers and errors
volatile uint8_t kiss_frame_rx_buffer[KISS_FRAME_BUFFER_SIZE];
int kiss_frame_rx_buffer_idx = 0;
bool kiss_rx_frame_ready = false;

// KISS Standard TNC Config values
uint8_t kiss_config_persistence = 63;
uint8_t kiss_config_slottime = 100;
uint8_t kiss_config_txdelay = 0;
uint8_t kiss_config_txtail = 0;
bool kiss_config_fullduplex = false;

// Additional Config unique to this implementation (using SETHW command)
uint8_t kiss_config_loopback = 0;
uint8_t kiss_config_tone = 0;
uint8_t kiss_config_txinhibit = 0;
uint8_t kiss_config_txforce = 0;
uint8_t kiss_config_audiofeedback = 0;

// TNC Active flag
bool kissTNCActive = false;

static void handleKISSControlFrame(volatile uint8_t *dataBuf);
static void parseKISSDataPacket(volatile uint8_t *dataBuf, int bytes);
static void encodeKISSDataFrame(volatile uint8_t *dataBuf, int bytes);

void tickKISSTNC(void)
{
	if(kissTNCActive)
	{
		tickHDLCModem();
		//After the modem has done it's thing, do we have any RX packets that need KISS encoding?
		if(hdlc_rx_frame_ready)
		{
			encodeKISSDataFrame(hdlc_frame_rx_buffer, hdlc_frame_rx_buffer_idx);
		}
	}
}

//Enable & Activate/Initialize the KISS TNC
void enableKISSTNC(void)
{
	flushKISSRxFrameBuffer();
	flushKISSTxFrameBuffer();
	kiss_error_counter_buffer_overflow = 0;
	kissTNCActive = true;
	kiss_frame_start_skipped = false;
}

//Disable & Deactivate the KISS TNC
void disableKISSTNC(void)
{
	flushKISSRxFrameBuffer();
	flushKISSTxFrameBuffer();
	kissTNCActive = false;
}

//Feed a Packet of Data to be Transmitted to the KISS TNC, to be appended to the queue
//(the provided data must be encoded in KISS async protocol)
bool sendKISSTxFrameDataPacket(volatile uint8_t *dataBuf, int bytes)
{
	int packetStartOffset = 0;
	//Make sure we're active, and not currently sitting on a full frame awaiting transmission
	if(kissTNCActive && !hdlc_tx_frame_ready)
	{
		//Ensure the buffer has room for the incoming data
		if(hdlc_frame_tx_buffer_idx+bytes <= HDLC_FRAME_BUFFER_SIZE)
		{
			//So here we've received a packet of data. The packet may not contain a full KISS frame
			//Incoming COM driver has only 64byte buffer that it puts in a request for example.
			//So we need to do some parsing and tracking of partial frames...

			//First we need to check, are we at the beginning of the buffer?
			if(hdlc_frame_tx_buffer_idx==0)
			{
				//If so then we are at the beginning of a new frame (or we should be anyway)
				//Now, do we see an FEND at the beginning of the packet (or did we previously start multiple fend packets?)
				if(dataBuf[0]==KISS_FEND || kiss_frame_start_skipped)
				{
					//Yes? Ok great, so we're starting a new frame as expected.
					//But... First we need to see if we have repeating FENDs for sync (must disregard duplicates)
					while(dataBuf[packetStartOffset]==KISS_FEND && packetStartOffset < bytes) packetStartOffset++;
					if(packetStartOffset+1 >= bytes)
					{
						//The whole packet was FEND... So we can just disregard it entirely
						//However we do need to track that this happened, in case they sent exactly one packet of FEND and then we get data
						//That way we know we still got an FEND to start the frame.
						kiss_frame_start_skipped = true;
						return true;
					}else{
						//Ok so we've seen a start, and we've filtered duplicate FEND bytes...
						//Is it a data packet? or control packet?
						if(dataBuf[packetStartOffset]==KISS_DF){
							//Data packet, ok so we'll parse contents...
							parseKISSDataPacket(dataBuf+packetStartOffset+1, bytes-packetStartOffset-1); //Stripping off the FEND start flag(s) and Command
							return true;
						}else{
							//Control packet? Ok let's handle the control packet:
							handleKISSControlFrame(dataBuf+packetStartOffset);
							return true;
						}
					}
				}else{
					//NO? Then something's wrong... We're missed data or the stream is corrupt.
					//Count a packet error and fail out
					kiss_error_counter_frame++;
					flushKISSTxFrameBuffer();
					return false;
				}
			} else {
				//If not, then we're appending to a frame, parse the whole thing
				parseKISSDataPacket(dataBuf, bytes);
				return true;
			}
		}else{
			//We've overrun the buffer, so count the error, and flush the buffer...
			kiss_error_counter_buffer_overflow++;
			flushKISSTxFrameBuffer();
			return false;
		}
	}else{
		return false;
	}
}

//Internal function used to parse the data in a packet being prepped for tx
static void parseKISSDataPacket(volatile uint8_t *dataBuf, int bytes)
{
	//At this point we know the packet we're receiving is able to fit in the buffer
	//We also know that it's supposed to be data destined for an HDLC frame (we've skipped the initial header)
	//And it MAY contain an FEND which should be the end of the frame.
	//It may also contain some FESC and transposed FEND/FESC bytes which we need to fix in transit.
	for(int i=0;i<bytes;i++)
	{
		if(dataBuf[i]==KISS_FEND)
		{
			//We just found the end of the frame. Woohoo! Signal it and bail!
			hdlc_tx_frame_ready = true;
			kiss_stats_bytes_tx += i;
			return;
		}
		if(dataBuf[i]==KISS_FESC)
		{
			//Escape, so skip ahead in the stream and transpose as needed.
			i++;
			hdlc_frame_tx_buffer[hdlc_frame_tx_buffer_idx] = dataBuf[i]==KISS_TFEND ? KISS_FEND : KISS_FESC;
			hdlc_frame_tx_buffer_idx++;
		}else{
			//Nothing to see here, just copy to the tx buffer.
			hdlc_frame_tx_buffer[hdlc_frame_tx_buffer_idx] = dataBuf[i];
		}
	}
	kiss_stats_bytes_tx += bytes;
}

//Internal function used to encode data into KISS async protocol to be sent to the "host" (rx buffer)
//Expects the data in dataBuf to be received HDLC data from the modem.
//Assumption is that this is an entire buffer (whole frame) at once.
static void encodeKISSDataFrame(volatile uint8_t *dataBuf, int bytes)
{
	//Start off by putting KISS_FEND and KISS_DF header on the frame
	kiss_frame_rx_buffer[0] = KISS_FEND;
	kiss_frame_rx_buffer[1] = KISS_DF;
	kiss_frame_rx_buffer_idx=2;

	//Now we iterate through the data in the buffer
	for(int i=0;i<bytes;i++)
	{
		//Encode depending on content in the payload
		if(dataBuf[i]==KISS_FEND)
		{
			//Encode FEND byte in the data payload
			kiss_frame_rx_buffer[kiss_frame_rx_buffer_idx] = KISS_FESC;
			kiss_frame_rx_buffer[kiss_frame_rx_buffer_idx+1] = KISS_TFEND;
			kiss_frame_rx_buffer_idx+=2;
		}else if(dataBuf[i]==KISS_FESC){
			//Encode FESC byte in the data payload
			kiss_frame_rx_buffer[kiss_frame_rx_buffer_idx] = KISS_FESC;
			kiss_frame_rx_buffer[kiss_frame_rx_buffer_idx+1] = KISS_TFESC;
			kiss_frame_rx_buffer_idx+=2;
		}else{
			//Encode the raw buffer byte
			kiss_frame_rx_buffer[kiss_frame_rx_buffer_idx] = dataBuf[i];
		}
	}

	//Finally let's stick a tail on the end:
	kiss_frame_rx_buffer[kiss_frame_rx_buffer_idx] = KISS_FEND;
	kiss_frame_rx_buffer_idx++;

	//Count the stats
	kiss_stats_bytes_rx+=kiss_frame_rx_buffer_idx;
	kiss_stats_frames_rx++;

	//And now we can flag that there is an RX packet ready!
	kiss_rx_frame_ready = true;
}

//Internal function which handles incoming "command and control" frames on KISS async protocol
static void handleKISSControlFrame(volatile uint8_t *dataBuf)
{
	switch(dataBuf[0])
	{
		case KISS_TXDELAY:
			kiss_config_txdelay = dataBuf[1];
			break;
		case KISS_P:
			kiss_config_persistence = dataBuf[1];
			break;
		case KISS_SLOTTIME:
			kiss_config_slottime = dataBuf[1];
			break;
		case KISS_TXTAIL:
			kiss_config_txtail = dataBuf[1];
			break;
		case KISS_FULLDUPLEX:
			kiss_config_fullduplex = (dataBuf[1]!=0);
			break;
		case KISS_SETHW:
			switch(dataBuf[1])
			{
				case KISS_CFG_ADDR_LOOPBACK:
					kiss_config_loopback = dataBuf[2];
					break;
				case KISS_CFG_ADDR_TXINHIBIT:
					kiss_config_txinhibit = dataBuf[2];
					break;
				case KISS_CFG_ADDR_TXFORCE:
					kiss_config_txforce = dataBuf[2];
					break;
				case KISS_CFG_ADDR_AUDIOFEEDBACK:
					kiss_config_audiofeedback = dataBuf[2];
					break;
				case KISS_CFG_ADDR_TONE:
					kiss_config_tone = dataBuf[2];
					break;
			}
			break;
	}
	kiss_stats_frames_tx++;
}

//Flushes the RX buffer from the TNC
//Call this from the "host" to indicate you have read the buffer contents
//Will flush the buffer and remove the data ready flag
//Allows next frame to be received
void flushKISSRxFrameBuffer(void)
{
	//Flush the KISS Frame Buffer
	kiss_frame_rx_buffer_idx = 0;
	kiss_rx_frame_ready = false;
	//Then pass through to HDLC frame buffer
	flushHDLCRxFrameBuffer();
}

//Flushes the TX buffer from the TNC
//This is primarily used as an internal function as part of the packet handling code
//But can also be used by the host in the event for some reason known malformed data has made it into the buffer.
//Essentially resets the buffer and logic.
void flushKISSTxFrameBuffer(void)
{
	//Just pass through for now
	flushHDLCTxFrameBuffer();
}

