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
extern char* CMDS (char a[], char * saveName);
extern void csk_mhx_pwr_on(void); // defined in csk_mhx.c
extern void csk_mhx_open(void); // defined in csk_mhx.c
extern void RSSI_setTelem(char *a, int startPos);
extern void RSSI_setConfigRecent(char *a, int startPos);
extern long long getMissionClock(void);

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
	if(n>255) n=255;
	return(HeTrans255(inpt, n));
//	char HeOut[n+10];
//	HeOut[0]='H';
//	HeOut[1]='e';
//	HeOut[2]=0x10;
//	HeOut[3]=0x03; //Transmit.
//	HeOut[4]=0x00;
//	HeOut[5]=(unsigned char) (n&0xFF); //n=Size of payload.
//	
//	HeCkSum(HeOut,6); //This will append two bytes to the end.
//	
//	int i=0;
//	while(i<n) {
//		HeOut[i+8]=inpt[i]; //Copy Payload into buffer.
//		i++;
//	}
//	HeCkSum(HeOut,8+n); //This will append two bytes to the end.
//	if(OSReadBinSem(BINSEM_HEON_P)) {
//		for(i=0;i<(10+n);i++)
//			csk_uart1_putchar(HeOut[i]);
//	}
//	else { 
//		csk_uart0_puts("Failed Transmit, radio disabled.\r\n");
//	}
//	return 1;
}

//Three ways to save, to avoid collisions.
static char HeSaveData[10]; //char, filename.
static char HeSaveData2[10]; //char, filename.
static char HeSaveData3[10]; //char, filename.
static char saveRSSIOnly=0;
// MYCALL and UNPROTO are used to authenticate the call sign
// The "2*" is needed because the AX.25 header are unsigned
// hex, and I'm too lazy to figure out the casts.  (MAS)
static unsigned char MYCALL[6] = {2*CALL0, 2*CALL1, 2*CALL2, 
		2*CALL3, 2*CALL4, 2*CALL5};
static unsigned char UNPROTO[6] = {2*GROUND0, 2*GROUND1, 2*GROUND2, 
		2*GROUND3, 2*GROUND4, 2*GROUND5}; 

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
		// Time stamp
		unsigned long long mc=getMissionClock();
		if(mc>=999999999) mc=999999999;
		sprintf(strTmp,"%09llu",mc);
		f_write(strTmp,1,9,aFile);
		if(saveRSSIOnly) {
			f_write(a+15,1,1,aFile);
			saveRSSIOnly=0; //Prevents this feature getting stuck on.
			//Add timestamp here? mins,secs,hundreths of secs from rtc?
		}
		else f_write(a,1,num,aFile);
		f_close(aFile);
		RSSI_setTelem(a, 8);	// Write to the RSSI task (which stores it)
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
		if (
			((unsigned char)a[pos+startPos] != MYCALL[pos]) 
			|| ((unsigned char) a[pos+7+startPos] != UNPROTO[pos])) {
			return(0);
		}
	}
	return(1);
}

void task_externalcmdsMHX(void) {
  	static char a[400];
  	static unsigned char check;
  	static unsigned int pos;
  	static unsigned char delay;
  	static unsigned char size;
	static int DELAY1 = 15;
	static int DELAY2 = 600;
	OSSignalBinSem(BINSEM_CLEAR_TO_SEND_P);

	while(1) {
		// Wait
		while(!csk_uart1_count() > 0) OS_Delay(1);
		check=0;
 		while(csk_uart1_count() > 0 && !check) {
			if(csk_uart1_getchar()=='H') {
				check=1;
			}
		}
		if(check) {
			// This looks like a packet. Make sure that no one else is sending
			// until we finish reading this one
//			OSTryBinSem(BINSEM_CLEAR_TO_SEND_P);
			pos=0;
			delay=0;
			while(pos<7 && delay<DELAY1) {
				if(!csk_uart1_count()) {
					OS_Delay(1);
					delay++;
				}
				else {
					a[pos]=csk_uart1_getchar();
					pos++;
				}
			}
			if(delay<DELAY1) {
				delay=0;
				if(a[0]=='e' && a[1]==0x20) {
					// The helium is telling us something
					if (a[2]==0x04 && a[3]==0x00) { //We got a receive Packet!
						size= (unsigned char) (a[4]);
						pos=0;
						while(pos<size && delay<DELAY2) {
							if(!csk_uart1_count()){
								OS_Delay(1);
								delay++;
							}
							else {
								a[pos]=csk_uart1_getchar();
								pos++;
							}
						}
						if(delay<DELAY2 && size>0) { //We got the whole message!
							if(callsignCheck(a, 0)) {
								//Checking the callsign above
								a[size-3]=0;
OS_Delay(10);								
								CMDS(((char*) (a))+16,0);
							}
						} 
					} else if(a[2]!=0x04 && a[2]!=0x03) { 
						// Not an RF transmit/receive; must be an internal command
						// Hand it off to HeBroadcastOrSave to handle
						for(pos=7;pos>0;pos--) a[pos]=a[pos-1];
						a[0]='H'; //Replace the 'H' at the beginning of the string for displayment purposes.
						pos=8;
						if(a[4]==0 && a[5]!=0) {//Things other than "Received Data" that have payloads to CDH.
							size=(unsigned char) (a[5]);
							while(pos<size+10 && delay<DELAY2) {
								if(!csk_uart1_count()){
									OS_Delay(1);
									delay++;
								}
								else {
									a[pos]=csk_uart1_getchar();
									pos++;
								}
							}
							if(delay<DELAY2) { //We got the whole message!
OS_Delay(10);
								int i;
								for(i=0;i<pos;i++) csk_uart0_putchar(a[i]);
								i=0;
								while(pos>255) {
									HeBroadcastOrSave(((char*) (a))+i,255); //HeTrans255(((char*) (a))+i,255);
									i=i+255;
									pos=pos-255;
								}
								if(pos) {
									HeBroadcastOrSave(((char*) (a))+i,pos); //HeTrans255(((char*) `	1(a))+i,pos);
								}
							}
						} else { //We only have a header to transmit.
							int i;
							for(i=0;i<pos;i++) csk_uart0_putchar(a[i]);
							HeBroadcastOrSave(a,pos); //HeTrans255(a,pos);
						}
					}
				} // End checking for 'e' and 0x20
			} // End of delay test
		} // End of check for start of packet

		// We have finished everything we're checking for. Signal that
		// it's clear to send
		OSSignalBinSem(BINSEM_CLEAR_TO_SEND_P);
	}	// End while(1)
} /* task_externalcmds() */
