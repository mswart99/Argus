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
#include "task_HeListen.h"
#include "helium.h"

// Pumpkin CubeSat Kit headers 
#include "csk_io.h"
#include "csk_uart.h"
#include "csk_sd.h"
#include "thin_usr.h"

// Pumpkin Salvo headers
#include "salvo.h"
extern char* CMDS (char a[], char * saveName);
extern void HeBroadcastOrSave(char* a, int num);
extern char HeTrans255Str(char *a);

// MYCALL and UNPROTO are used to authenticate the call sign
// The "2*" is needed because the AX.25 header are unsigned
// hex, and I'm too lazy to figure out the casts.  (MAS)
static unsigned char MYCALL[6] = {2*CALL0, 2*CALL1, 2*CALL2, 
		2*CALL3, 2*CALL4, 2*CALL5};
static unsigned char UNPROTO[6] = {2*GROUND0, 2*GROUND1, 2*GROUND2, 
		2*GROUND3, 2*GROUND4, 2*GROUND5}; 

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

void task_HeListen(void) {
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
			OSTryBinSem(BINSEM_CLEAR_TO_SEND_P);
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
