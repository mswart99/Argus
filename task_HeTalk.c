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
#include "task_HeTalk.h"
#include "helium.h"

// Pumpkin CubeSat Kit headers 
#include "csk_io.h"
#include "csk_uart.h"
#include "csk_sd.h"
#include "thin_usr.h"

// Pumpkin Salvo headers
#include "salvo.h"
//extern char* CMDS (char a[], char * saveName);
extern void RSSI_setTelem(char *a, int startPos);
extern void RSSI_setConfigRecent(char *a, int startPos);
extern void getMissionClockString(char *a);

/* Computes the Helium checksum on the first N chars in the 
 * buffer. It will add the two checksum bytes at the end
 */
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

static int nUse = 24;
// Transmit the first n characters of the input string, zeroes included,
// up to 255 characters (maximum size of the helium string)
char HeTrans255(char* inpt, int n) {
	// Only broadcast if the radio is on. Otherwise, don't bother
	static char HeOut[265];	// 255 data characters + 8 header + 2 checksum
	if(OSReadBinSem(BINSEM_HEON_P)) {
		if(n>255) n=255;
		if(n<0) n=0;
		n = nUse;
		HeOut[0]=SYNC1;
		HeOut[1]=SYNC2;
		HeOut[2]=HE_COMMAND;
		HeOut[3]=TRANSMIT_DATA; //Transmit.
		HeOut[4]=0x00;          // No parameter
		HeOut[5]=(unsigned char) (n&0xFF); //n=Size of payload.
		
		HeCkSum(HeOut,6); //This will append two bytes to the end.
		
		int i=0;
		// Copy the payload data into the buffer (avoids messing with input pointer)
		while(i<n) {
			HeOut[i+8]=inpt[i]; //Copy Payload into buffer.
			i++;
		}
		HeCkSum(HeOut,8+n); //This will append two bytes to the end.
		for(i=0;i<(10+n);i++) {
				csk_uart1_putchar(HeOut[i]);
		}			
		OSSignalMsgQ(MSGQ_HETX_P, (OStypeMsgP) HeOut);
		// -------- THIS ONE PASSES THE STRING TO THE QUEUE ---------------------
//		char *ugh2 = "Ugh2";
//		OSSignalMsgQ(MSGQ_HETX_P, (OStypeMsgP) ugh2);

//		static char ugh[4];
//		sprintf(ugh, "Ugz!");
//		OSSignalMsgQ(MSGQ_HETX_P, (OStypeMsgP) ugh);
//		static char hetest[255];
//		for (i=0; i < (n+2); i++) {
//			hetest[i] = HeOut[i+8];
//		}
//		OSSignalMsgQ(MSGQ_HETX_P, (OStypeMsgP) hetest);
//		if (tst == OSNOERR) {
//			return 1;
//		} else if (tst == OSERR_BAD_P) {
//			HeOut[8] = 'b';
//		} else if (tst == OSERR_EVENT_FULL) {
//			HeOut[8] = 'f';
//		} else if (tst == OSERR_EVENT_CB_UNINIT) {
//			HeOut[8] = 'u';
//		} else if (tst == OSERR_EVENT_BAD_TYPE) {
//			HeOut[8] = 't';
//		} else {
//			HeOut[8] = '?';
//		}
//		HeOut[5] = 0x01;
//		HeCkSum(HeOut,6); //This will append two bytes to the end.
//		HeCkSum(HeOut,9); //This will append two bytes to the end.
//		for(i=0;i<11;i++)
//				csk_uart1_putchar(HeOut[i]);
				
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
}

//Three ways to save, to avoid collisions.
static char HeSaveData[10]; //char, filename.
static char HeSaveData2[10]; //char, filename.
static char HeSaveData3[10]; //char, filename.
static char saveRSSIOnly=0;

void setRSSISave(char zeroOrOne) {
	saveRSSIOnly=zeroOrOne;
}

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

/* This function is called when the data received from the Helium is not a 
 * RX packet 0x04. We check the packet id (a[3]) against our event handling
 * options. If it doesn't match, default is to broadcast an echo.
 */
void HeBroadcastOrSave(char* a, int num) {
	if(HeSaveData[0]==a[3]) { // Handle Telemetry requests (RSSI or full telem)
		F_FILE* aFile=f_open(((char*) (HeSaveData))+1,"a");
		char strTmp[32];
		// Time stamp
		getMissionClockString(strTmp);
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

void task_HeTalk(void) {
//  	static char a[400];
//  	static unsigned char check;
//  	static unsigned int pos;
//  	static unsigned char delay;
//  	static unsigned char size;
//	static int DELAY1 = 15;
//	static int DELAY2 = 600;
//	OSSignalBinSem(BINSEM_CLEAR_TO_SEND_P);
    static OStypeMsgP msgP;
//	static char n;
//    static char* packet;
//    static unsigned int dataLength;
//	static char *ugh = "UGH2";
//	sprintf(ugh, "UGH2");

	while(1) {
        /* Wait forever for a message to transmit to the Helium. */
        OS_WaitMsgQ(MSGQ_HETX_P, &msgP, OSNO_TIMEOUT);
        int i=0;
//		while (msgP+i != '\0') {
//			csk_uart1_putchar((char) (msgP+i));
//			i++;
//		}
		static char HeOut2[265];
		sprintf(HeOut2, (char *) msgP);
		OS_Delay(50);
        for (i=0; i < (10+nUse); i++) {
            csk_uart1_putchar(HeOut2[i]);
        }
//		HeOut[0]=SYNC1;
//		HeOut[1]=SYNC2;
//		HeOut[2]=HE_COMMAND;
//		HeOut[3]=TRANSMIT_DATA; //Transmit.
//		HeOut[4]=0x00;          // No parameter
//		HeOut[5]=0x14; //n=Size of payload.
//		HeCkSum(HeOut,6); //This will append two bytes to the end.
//		sprintf(HeOut, "%c%c%c%c%c%c%c%c%s", SYNC1, SYNC2, HE_COMMAND, 
//			TRANSMIT_DATA, 0x00, 0x04, 0x00, 0x00, (char *) msgP);
//		HeOut[8] = msgP;
//		for (i=0; i < 30; i++) {
//			HeOut[i]=  msgP[i];
//		}
//		HeCkSum(HeOut, 8+HeOut[5]);
//		int n = strlen(HeOut2);
		OS_Delay(50);
		static char HeOut3[265];
		sprintf(HeOut3, (char *) msgP);
		HeOut3[5]=0x01; //n=Size of payload.
		HeCkSum(HeOut3,6); //This will append two bytes to the end.
		HeOut3[8]='A';
		HeCkSum(HeOut3,9);
        for (i=0; i < 11; i++) {
            csk_uart1_putchar(HeOut3[i]);
        }
		OS_Delay(50);

//		sprintf(HeOut, "%c%c%c%c%c%c%c%c%s", SYNC1, SYNC2, HE_COMMAND, 
//			TRANSMIT_DATA, 0x00, 0x04, 0x00, 0x00, (char *) msgP);
//		HeCkSum(HeOut,6); //This will append two bytes to the end.
//		HeOut[8] = msgP;
//		for (i=0; i < 30; i++) {
//			HeOut[i]=  msgP[i];
//		}
//		HeCkSum(HeOut, 8+HeOut[5]);
//		int n = strlen(HeOut2);


        // We have finished our time with the radio. Signal that
        // it's clear to send
        OSSignalBinSem(BINSEM_CLEAR_TO_SEND_P);
    }	// End while(1)
} /* task_externalcmds() */
