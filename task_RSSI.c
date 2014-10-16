/******************************************************************************
(C) Copyright Pumpkin, Inc. All Rights Reserved.

This file may be distributed under the terms of the License
Agreement provided with this software.

THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE.

$Source: C:\\RCS\\D\\Pumpkin\\CubeSatKit\\Example\\all\\all\\CubeSatKit_Dev_Board\\Test\\Test1\\task_RSSI.c,v $
$Author: aek $
$Revision: 3.1 $
$Date: 2009-11-02 00:45:07-08 $

******************************************************************************/
#include "main.h"
#include "task_RSSI.h"
#include "helium.h"

// Pumpkin CubeSat Kit headers
#include "csk_io.h"
#include "csk_uart.h"
#include "csk_sd.h"
#include "thin_usr.h"

// Pumpkin Salvo headers
#include "salvo.h"

extern void setRSSISave(char zeroOrOne);
extern void setHeSaveData(char* a);
extern void setHeSaveData3(char* a);
extern void HeCkSum(char* buffer, int n);
extern char* asciifiedArray(char* a, int aLen);
extern char* asciifiedArrayNoSpace(char* a, int aLen);

//extern char HeTrans255(char* input, int n);
static char saveHeTelemCommand[10]={0x07,'R','S','S','I',0};
static char saveHeConfigCommand[10]={0x05,'C','O','N','F','I','G',0};

static char HeGetTelem[8]={SYNC1,SYNC2,0x10,TELEMETRY_QUERY,0x00,0x00,0,0};
static char HeGetConfig[8]={SYNC1,SYNC2,0x10,GET_TRANSCEIVER_CONFIG,0x00,0x00,0,0};
static char HeTelem[HE_TELEM_LEN]={'e','m','p','t','y',0};	// The local stored value of telemetry
static char HeConfigRecent[HE_CONFIG_LEN]={'n','o',0};	// The local stored value of configuration

/** Copy the values of a into the stored value of configuration.
 */
void RSSI_setConfigRecent(char *a, int startPos) {
	static int i;
	for (i=0; i < HE_CONFIG_LEN; i++) {
		HeConfigRecent[i] = a[i+startPos];
	}
}

/** Copy the values of a into the telemetry, starting at startPos
 */
void RSSI_setTelem(char* a, int startPos) {
	static int i;
	for (i=0; i < HE_TELEM_LEN; i++) {
		HeTelem[i] = a[i+startPos];
	}
}

/** Return the telemetry pointer */
char* RSSI_getConfig(int charOrAscii) {
    if (charOrAscii == 0) {
        return HeConfigRecent;
	} else if (charOrAscii == 1) {
	    return asciifiedArray(HeConfigRecent, HE_CONFIG_LEN);
	}
	return asciifiedArrayNoSpace(HeConfigRecent, HE_CONFIG_LEN);
}

/* Returns the Helium telemetry as a char array if asciiOrHex = 0,
 * otherwise, returns an ASCII-fied version of the char array.
 */
char* RSSI_getTelem(int charOrAscii) {
    if (charOrAscii == 0) {
        return HeTelem;
    } else if (charOrAscii == 1) {
	    return asciifiedArray(HeTelem, HE_TELEM_LEN);
	}
	return asciifiedArrayNoSpace(HeTelem, HE_TELEM_LEN);
}

/** Sends the "get telemetry" command to the Helium,
 * and configures the event handling in task_externalcommandsMHX
 * to save to the RSSI file.
 */
void askNSaveRSSI() {
	setRSSISave(1);
	setHeSaveData(saveHeTelemCommand);
	int i;
	for(i=0;i<8;i++) {
		csk_uart1_putchar(HeGetTelem[i]);
	}
}

/** Sends the "get configuration" command to the Helium,
 * and configures the event handling in task_externalcommandsMHX
 * to save to the Config file.
 */
void askNSaveConfig() {
	setHeSaveData3(saveHeConfigCommand);
	int i;
	for(i=0;i<8;i++) {
		csk_uart1_putchar(HeGetConfig[i]);
	}
}

void task_RSSI(void) {
  // Finish formatting the telem/config commands
  HeCkSum(HeGetTelem,6);
  HeCkSum(HeGetConfig, 6);
  static unsigned int count, i;

  OS_Delay(250);OS_Delay(250);

  // Measure RSSI more often after deployment
  if(!OSReadBinSem(BINSEM_DEPLOYED_P)) {
	  count=0;
	  // Confirm configuration
	  OS_WaitBinSem(BINSEM_CLEAR_TO_SEND_P, OSNO_TIMEOUT);
	  askNSaveConfig();
	  // Measure RSSI every half-second for the first 8 minutes
	  while(count<8*2*60) {
		OS_Delay(50);
	    OS_WaitBinSem(BINSEM_CLEAR_TO_SEND_P, OSNO_TIMEOUT);
		askNSaveRSSI();
		count++;
	  }
	  // Continue measuring RSSI every 5 seconds until deployed
	  while(!OSReadBinSem(BINSEM_DEPLOYED_P)) {
		OS_Delay(250);OS_Delay(250);
	    OS_WaitBinSem(BINSEM_CLEAR_TO_SEND_P, OSNO_TIMEOUT);
		askNSaveRSSI();
	  }	
  }

  while (1) {
    /** Get RSSI every minute, and configuration every 10 */
	OS_WaitBinSem(BINSEM_CLEAR_TO_SEND_P, OSNO_TIMEOUT);
	askNSaveConfig();
	int offset = 4;
	for (count=0; count < 10*offset; count++) {
		OS_WaitBinSem(BINSEM_CLEAR_TO_SEND_P, OSNO_TIMEOUT);
		askNSaveRSSI();
		for (i=0; i<24/offset; i++) {
			// Wait 2.5 seconds
			OS_Delay(250);
		}
	}
  } /* while */
} /* task_RSSI() */


