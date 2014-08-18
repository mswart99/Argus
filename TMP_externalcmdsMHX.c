/******************************************************************************
(C) Copyright Pumpkin, Inc. All Rights Reserved.

This file may be distributed under the terms of the License
Agreement provided with this software.

THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!I2C--Not 5sec~~~~~~~~~~~~~~~~~~~~~~~
$Source: C:\\RCS\\D\\Pumpkin\\CubeSatKit\\Example\\all\\all\\CubeSatKit_Dev_Board\\Test\\Test1\\task_5sec.c,v $
$Author: aek $
$Revision: 3.1 $
$Date: 2009-11-02 00:45:07-08 $

******************************************************************************/
#include "main.h"
#include "task_externalcmdsMHX.h"
#include "helium.h"

// Pumpkin CubeSat Kit headers 
#include "csk_io.h"
#include "csk_uart.h"
#include "csk_sd.h"
#include "thin_usr.h"

// Pumpkin Salvo headers
#include "salvo.h"
extern void CMDS (char a[], char * saveName);
extern void csk_mhx_pwr_on(void); // defined in csk_mhx.c
extern void csk_mhx_open(void); // defined in csk_mhx.c
extern void RSSI_setTelem(char *a, int startPos);
extern void RSSI_setConfigRecent(char *a, int startPos);

static char MYCALL[6] = {CALL0, CALL1, CALL2, CALL3, CALL4, CALL5};
static char UNPROTO[6] = {GROUND0, GROUND1, GROUND2, GROUND3, GROUND4, GROUND5}; 
static char heBuffer[BUFFER_LEN];


void HeCkSum(char* buffer, int n) {
	//This will add two bytes to the end of buffer.
	unsigned char* ubuffer=(unsigned char*) buffer;
	int i=2;
	unsigned char ck_a=0;
	unsigned char ck_b=0;
	while(i<n) {
		ck_a=ck_a+ubuffer[i];
		ck_b=ck_b+ck_a;
		i++;
	}
	ubuffer[n]=ck_a;
	ubuffer[n+1]=ck_b;
	//ubuffer[n+2]=0;
}

// Transmit the first n characters of the input string, zeroes included,
// up to 255 characters (maximum size of the helium string)
char HeTrans255(char* inpt, int n) {
	// Only broadcast if the radio is on. Otherwise, don't bother
	if(OSReadBinSem(BINSEM_HEON_P)) {
		if(n>255) n=255;
		if(n<0) n=0;
		char HeOut[n+10];
		HeOut[0]='H';
		HeOut[1]='e';
		HeOut[2]=0x10;
		HeOut[3]=0x03; //Transmit.
		HeOut[4]=0x00;
		HeOut[5]=(unsigned char) (n&0xFF); //n=Size of payload.
		
		HeCkSum(HeOut,6); //This will append two bytes to the end.
		
		int i=0;
		// Copy the payload data into the buffer (avoids messing with input pointer)
		while(i<n) {
			HeOut[i+8]=inpt[i]; //Copy Payload into buffer.
			i++;
		}
		HeCkSum(HeOut,8+n); //This will append two bytes to the end.
			for(i=0;i<(10+n);i++)
				csk_uart1_putchar(HeOut[i]);
	} else { 
		csk_uart0_puts("Failed Transmit, radio disabled.\r\n");
		return 0;
	}
	return 1;
}

/** Transmit an entire string (go until the null termination character is found
 */
char HeTrans255Str(char* inpt) {
	// Find the null termination character, then call HeTrans255
	int n=0;
	while(inpt[n]) n++; //Find the null termination char.
//	HeTrans255(inpt, n);
	if(n>255) n=255;
	char HeOut[n+10];
	HeOut[0]='H';
	HeOut[1]='e';
	HeOut[2]=0x10;
	HeOut[3]=0x03; //Transmit.
	HeOut[4]=0x00;
	HeOut[5]=(unsigned char) (n&0xFF); //n=Size of payload.
	
	HeCkSum(HeOut,6); //This will append two bytes to the end.
	
	int i=0;
	while(i<n) {
		HeOut[i+8]=inpt[i]; //Copy Payload into buffer.
		i++;
	}
	HeCkSum(HeOut,8+n); //This will append two bytes to the end.
	if(OSReadBinSem(BINSEM_HEON_P)) {
		for(i=0;i<(10+n);i++)
			csk_uart1_putchar(HeOut[i]);
	}
	else { 
		csk_uart0_puts("Failed Transmit, radio disabled.\r\n");
	}
	return 1;
}

//Three ways to save, to avoid collisions.
static char HeSaveData[10]; //char, filename.
static char HeSaveData2[10]; //char, filename.
static char HeSaveData3[10]; //char, filename.
static char saveRSSIOnly=0;

void setHeSaveData(char* a) {
	int i;
	for(i=0;i<10;i++) HeSaveData[i]=a[i];
}
void setHeSaveData2(char* a) {
	int i;
	for(i=0;i<10;i++) HeSaveData2[i]=a[i];
}
void setHeSaveData3(char* a) {
	int i;
	for(i=0;i<10;i++) HeSaveData3[i]=a[i];
}

void setRSSISave(char zeroOrOne) {
	saveRSSIOnly=zeroOrOne;
}

/* This function is called when the data received from the Helium is not a 
 * RX packet 0x04. We check the packet id (a[3]) against our event handling
 * options. If it doesn't match, default is to broadcast an echo.
 */
void HeBroadcastOrSave(char* a, int num) {
	if(HeSaveData[0]==a[3]) { // Handle Telemetry requests (RSSI or full telem)
		F_FILE* aFile=f_open(((char*) (HeSaveData))+1,"a");
		if(saveRSSIOnly) {
			f_write(a+15,1,1,aFile);
			saveRSSIOnly=0; //Prevents this feature getting stuck on.
			//Add timestamp here? mins,secs,hundreths of secs from rtc?
		}
		else f_write(a,1,num,aFile);
		f_close(aFile);
		RSSI_setTelem(a, 8);	// Write to the RSSI task (which stores it)
//		debugOut(TLM_DB, 3, a, num, 14);
		HeSaveData[0]=0; //Prevents saving the data more than once.
		return; //Prevents broadcasting after a save.
	}
	if(HeSaveData2[0]==a[3]) { // Handle Beacon data
		F_FILE* aFile=f_open(((char*) (HeSaveData2))+1,"a");
		f_write(a,1,num,aFile);
		f_close(aFile);
		HeSaveData2[0]=0; //Prevents saving the data more than once.
		return; //Prevents broadcasting after a save.
	}
	if(HeSaveData3[0]==a[3]) { // Handle He Config data
		F_FILE* aFile=f_open(((char*) (HeSaveData3))+1,"a");
//		debugOut(CFG_DB, 3, a, num, 34);
		f_write(a,1,num,aFile);
		f_close(aFile);
		RSSI_setConfigRecent(a, 8);	// Write to the RSSI task (which stores it)
		HeSaveData3[0]=0; //Prevents saving the data more than once.
		return; //Prevents broadcasting after a save.
	}
	HeTrans255(a,num);
}

/* Check that the first 13 characters of A match
 * the callsigns for authenticated commanding.
 */
int callsignCheck(char* a, int startPos) {
	int pos;
	for (pos = 0; pos < 6; pos++) {
		if ((a[pos+startPos] != MYCALL[pos]) 
			|| (a[pos+7+startPos] != UNPROTO[pos])) {
			return(0);
		}
	}
	return(1);
}

/* Increments the counter i by one and returns the new i, unless
 * 1) The passed value of i was == stopPos (returns -1)
 * 2) i has reached the end of the buffer (returns 0)
 * 
 */
int incrementCarefully(int i, int stopPos, int bufferLen) {
	// Check for stop-search conditions
	if (i == stopPos) {
		return -1;
	}
	i++;
	if (i == bufferLen) { // Wrap!
		return 0;
	}
	return(i);
}

// Step through the character array a, beginning at index startPos, until either
// 1) a[i] = charToFind (return i)
// 2) i = stopPos (return -1)
// If the end of the array is reached (aLen-1), the search wraps 
// around to 0 and continues.
int parseUntil(char* a, char charToFind, int aLen, int startPos, int stopPos) {
	int i = startPos;
	while (a[i] != charToFind) {
		// Check for stop-search conditions
		// Increment until we reach stopPos (which is the spot after the end of the data)
		i = incrementCarefully(i, stopPos, aLen);
		if (i == -1) {
			return -1;
		}
	} // End while -- keep looking
	// If we reach this point, a[i] = charToFind. Return that index.
	return i;
}

/** Convenience method to figure out how many characters are left to parse
 *  accounting for 'wrap' around the buffer. Note that readPos is parked AFTER the
 *  the last-read character in the buffer, but parsePos is parked AT the last-read
 *  character in the buffer. (It was easier to manage them this way in the rest of the
 *  algorithm. Honest!) So if parsePos = readPos -1, then the unparsedCharacterCount
 *  returns 0.
 *  
 *  Note also that readPos and parsePos should never equal one another. Doing so will
 *  lead to great confusion (i.e. errors) in the rest of the code. This function will
 *  return the value (bufferLength-1), in case you want to look for it.
 */
int unparsedCharacterCount(int parsePos, int readPos, int bufferLength) {
	if (readPos > parsePos) { // Easy. The read position is "after" the parse
		return(readPos - parsePos -1);
	} else { // Have to wrap around
		return(readPos - parsePos + bufferLength -1);
	}
}

/** Pull from the buffer until there's nothing left
 */
int readAllFromHelium(int readPos, int parsePos, int bufferLength) {
//	readPos = incrementCarefully(readPos, parsePos, bufferLength);
	int startRead = readPos;
	while (csk_uart1_count() && (readPos != -1)) {
		heBuffer[readPos] = csk_uart1_getchar();
		// Increment until we reach the spot before parsePos
		if (heBuffer[readPos] != 0) {
			readPos = incrementCarefully(readPos, parsePos-1, bufferLength);
			OS_Delay(1);
		}
		if (readPos - startRead > 40) {
			HeTrans255(heBuffer, readPos);
		}
	}
	if (readPos == -1) {
		if (parsePos == 0) {
			return(bufferLength-1);
		}
		return(parsePos-1);
	}
	return(readPos);
}

void clearBuffer(char* buf, int length) {
	int i;
	for (i=0; i < length; i++) {
		buf[i] = 0x00;
	}
}

void task_externalcmdsMHX(void) {
	static char payloadBuffer[256];
	int payloadPos;
	char packetType;
	unsigned int size;
	int readStatus = 0;
	// parsePos indicates the last-parsed character. It remains on that index
	int parsePos = BUFFER_LEN-1;
	// readPos indicates the END of the read buffer. The last-read character
	// is at (readPos-1)
	int readPos = 0;
	OSSignalBinSem(BINSEM_CLEAR_TO_SEND_P);
	while(0) {
		// Start by assuming it's clear to send commands to the Helium
//		OSSignalBinSem(BINSEM_CLEAR_TO_SEND_P);
		// Read all that we can. ReadPos is the position AFTER 
		// the last character in the buffer
		readPos = readAllFromHelium(readPos, parsePos, BUFFER_LEN);
		// We don't switch on readStatus, because we can run these consecutively
		if (readStatus == 0) {
			// Look for an ‘H’
			// parseUntil() will either return the index of the next 'H',
			// or -1 if we wrap around and catch up to readPos
			parsePos = parseUntil(heBuffer, SYNC1, BUFFER_LEN, parsePos, readPos);
			if ((parsePos == -1) || (parsePos == readPos)) {
				// Set parsePos at the last-parsed character (readPos-1)
				parsePos = readPos-1;
			} else { // We found the ‘H’!
				readStatus = 1;	// Look for the e
				clearBuffer(payloadBuffer, 256);
				payloadBuffer[0] = SYNC1;
			}
		}
		if (readStatus == 1) {
			// The next character should be ‘e’
			// We stopped at the last-read character, so we need to increment
			parsePos = incrementCarefully(parsePos, readPos, BUFFER_LEN);
			// If we get -1, it means we're at the end of the read buffer, and must wait
			// for more data
			if ((parsePos == -1) || (parsePos == readPos)) {
				// As before, we need to set parsePos to the last-parsed character
				parsePos = readPos-1; 
			} else { //if (parsePos != readPos) {
				if (heBuffer[parsePos] == SYNC2) {
					readStatus = 2;
					payloadBuffer[1] = SYNC2;
				} else {
					readStatus = 0;	// Back to the drawing board!
				}
			}
		} // We finish the readStatus 1 check and next check for readStatus 2
		if (readStatus == 2) {
			// The next character should be 0x20
			// We stopped at the last-read character, so we need to increment
			parsePos = incrementCarefully(parsePos, readPos, BUFFER_LEN);
			// If we get -1, it means we're at the end of the read buffer, and must wait
			// for more data
			if ((parsePos == -1) || (parsePos == readPos)) {
				parsePos = readPos - 1; // See above for explanation
			} else // Check for match on SYNC3
				if (heBuffer[parsePos] == SYNC3) {
					readStatus = 3;
					payloadBuffer[2] = SYNC3;
				} else {
					readStatus = 0;	// Back to the drawing board!
				}
		}
		// We have a header packet start. But do we have the rest of the header?
		// Should be five characters (can be wrapped around the buffer)
		if ((readStatus == 3) 
				&& (unparsedCharacterCount(parsePos, readPos, BUFFER_LEN) >=5)) {
			// We have the rest of the header loaded
			// parsePos is still at SYNC3, so we know how many to increment
			// The packet type is the next character
			parsePos = incrementCarefully(parsePos, readPos, BUFFER_LEN);
			packetType = heBuffer[parsePos];
			payloadBuffer[3] = packetType;
			// The size is two characters along
			parsePos = incrementCarefully(parsePos, readPos, BUFFER_LEN);
			payloadBuffer[4] = heBuffer[parsePos];
			parsePos = incrementCarefully(parsePos, readPos, BUFFER_LEN);
			size = heBuffer[parsePos];
			payloadBuffer[5] = (char) size;
			// We don't want the next two characters (checksums)
			parsePos = incrementCarefully(parsePos, readPos, BUFFER_LEN);
			payloadBuffer[6] = heBuffer[parsePos];
			parsePos = incrementCarefully(parsePos, readPos, BUFFER_LEN);
			payloadBuffer[7] = heBuffer[parsePos];
			// Ready for stage 4
			readStatus = 4;
			payloadPos = 8;
		} // If we don't have readStatus == 3 and 5 characters, we just wait
		// Are we pulling data?
		if ((readStatus == 4) && (parsePos != readPos-1)) {
			// Grab data until we can grab no more
			while ((payloadPos < size+8) && (unparsedCharacterCount(parsePos, readPos, BUFFER_LEN) > 0)) {
				// Read first, because we may be starting in the middle of a read
				parsePos = incrementCarefully(parsePos, readPos, BUFFER_LEN);
				payloadBuffer[payloadPos++] = heBuffer[parsePos];
			}
			if (payloadPos == size + 8) { // Got it all!
				readStatus = 5;
				payloadBuffer[payloadPos] = 0x00;
			} 
		}
		if (readStatus == 5) { // Evaluate the command
			if (packetType == RECEIVE_DATA) {
				HeTrans255(payloadBuffer, payloadPos);
				// Check callsign and execute
				if (callsignCheck(payloadBuffer, 8) == 1) {
					CMDS(((char*) (payloadBuffer))+17,0);
				} else {
					CMDS(((char*) (payloadBuffer))+8,0);
				}
			} else if ((packetType == GET_TRANSCEIVER_CONFIG)
					|| (packetType == TELEMETRY_QUERY)) {
				HeTrans255(payloadBuffer, payloadPos);
			// Send the entire array 
				//						// Dump to uart0 debug line
				//						pos = size + 8;
				//						int i;
				//						for(i=0;i<pos;i++) csk_uart0_putchar(a[i]);
				//						i=0;
				// Can only handle 255 byte packets
				//						while(pos>255) {
				//							HeBroadcastOrSave(((char*) (a))+i,255); //HeTrans255(((char*) (a))+i,255);
				//							i=i+255;
				//							pos=pos-255;
				//						}
				//						if(pos) {
				HeBroadcastOrSave(payloadBuffer,payloadPos); //HeTrans255(((char*) `	1(a))+i,pos);
			} else {
				// We don't do anything with the other commands
			}
			// We are finished with this packet. Reset
			readStatus = 0;
			payloadPos = 0;
		}	// End readStatus =5 work
	}	// End while(1)
} /* task_externalcmds() */
