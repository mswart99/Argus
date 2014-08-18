/******************************************************************************
(C) Copyright Pumpkin, Inc. All Rights Reserved.

This file may be distributed under the terms of the License
Agreement provided with this software.

THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE.

$Source: C:\\RCS\\D\\Pumpkin\\CubeSatKit\\Example\\all\\all\\CubeSatKit_Dev_Board\\Test\\Test1\\task_5sec.c,v $
$Author: aek $
$Revision: 3.1 $
$Date: 2009-11-02 00:45:07-08 $

******************************************************************************/
#include "main.h"
#include "task_Vand.h"

// Pumpkin CubeSat Kit headers
#include "csk_io.h"
#include "csk_uart.h"
#include "csk_sd.h"
#include "thin_usr.h"

// Pumpkin Salvo headers
#include "salvo.h"

//extern char * i2c_SnR(int address, char sending[], int num, char charArrayOfAppropriateSize[], int asciiOrHex);
//extern void i2c_sendz(char sending[]);
//extern char * i2c_readz(int num, char charArrayOfAppropriateSize[], int asciiOrHex);
//extern char send_i2c_byte(int data);
//extern void i2c_start(void);
//extern void reset_i2c_bus(void);
//extern unsigned char i2c_read_ack(void);
//extern unsigned char i2c_read_nack(void);
//extern void timeStamp(char* aString);
//extern long long getMissionClock(void);
//extern char HeTrans255Str(char* inpt);
//extern char HeTrans255(char* inpt, int n);
extern char * i2c_SnR_v2(int address, char sending[], int numSend, int numRec, char charArrayOfAppropriateSize[], int asciiOrHex);
extern char * asciifiedArray(char* a, int aLen);
extern char * asciifiedArrayNoSpace(char* a, int aLen);
//extern void i2c_sendz(char sending[], int num);
//extern void i2c_restart(void);
//extern char * i2c_readz(int num, char charArrayOfAppropriateSize[], int asciiOrHex);

//This applies to a specific CRC8: x^8+x^2+x^1+x^0
static char CRC8Table[256] = {
	0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
	0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
	0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
	0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
	0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
	0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
	0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
	0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
	0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
	0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
	0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
	0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
	0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
	0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
	0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
	0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
	0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
	0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
	0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
	0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
	0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
	0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
	0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
	0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
	0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
	0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
	0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
	0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
	0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
	0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
	0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
	0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};
static char vucTelem[VUC_TELEM_LEN];		// Stores the Telemetry data

//This applies to a specific CRC8: x^8+x^2+x^1+x^0, and incorporates a (0x20<<1+1) first (VUC's Address)!
char GenCRC8(void* dataV, int size) {
	//Size is in bytes.
	unsigned char* data=(unsigned char*) dataV;
	unsigned char crc=CRC8Table[0x40];
	while(size) {
		//crc=(*data) ^ CRC8Table[crc];
		crc=CRC8Table[crc ^ (*data)];
		data++;
		size--;
	}
	return crc;
}

/** Send an i2c command to the VUC. This function is a wrapper, adding the CRC8 error check
 * and calling the i2c function.
 * returns: the response from the i2c line in returnArray (native char, not ASCII-inated)
 */
void VUC(char a[], int numSend, char returnArray[], int numRet) {
	//For memory saving purposes, a must have at least one space capacity beyond numSend.
	a[numSend]=GenCRC8((char*)a,numSend);
	numSend++;
	i2c_SnR_v2(VUC_I2C_ADDRESS,a,numSend,numRet,returnArray,0);
}

/** Send an i2c command to the VUC. This function is a wrapper, adding the CRC8 error check
 * and calling the i2c function.
 * returns: the response from the i2c line in a ASCII-inated string, 
 *   	i.e. 0x20 0x4A becomes "20 4A ". Make sure that returnArray is at least
 *		3*numRet in length!
 */
void VUCHex(char a[], int numSend, char returnArray[], int numRet) {
	/* VUCHex overwrites a with the returned data
	*/
	a[numSend]=GenCRC8(a,numSend);
	numSend++;
	i2c_SnR_v2(VUC_I2C_ADDRESS,a,numSend,numRet,returnArray,1);
}

void Vand(char a[], int numSent, int numRet) {
	i2c_SnR_v2(VAND_I2C_ADDRESS,a,numSent,numRet,a,0);
}

void VandHex(char a[], int numSent, int numRet) {
	i2c_SnR_v2(VAND_I2C_ADDRESS,a,numSent,numRet,a,1);
}

static char VUC_basecommand[7] = {VUC_MSG_VER_HI, VUC_MSG_VER_LO, VUC_SW_VER_HI, VUC_SW_VER_LO, 
		0x00, 0x00, 0x00};

/** Send a command to the VUC with no arguments (just the two-byte command)
	returns: the N data characters from the VUC response will be written to retData
		and the error code character will be returned. Note that you need to know how
		many data characters your command will return!

	The VUC always responds with an 8-byte header
	VUC_MSG_VER_HI VUC_MSG_VER_LO VUC_SW_VER_HI VUC_SW_VER_LO reserved error code ACK_LEN_HI ACK_LEN_LO
	
	This header will NOT be returned. If you need to debug, use VUC() or VUCHex()
 */
char VUC_sendCommandNoArgs(char commandHi, char commandLow, int n, char* retData) {
	// Add the command specifics to our basecommand
	VUC_basecommand[4] = commandHi;
	VUC_basecommand[5] = commandLow;
	VUC_basecommand[6] = GenCRC8(VUC_basecommand,6);
	char tmp[8+n];
	i2c_SnR_v2(VUC_I2C_ADDRESS, VUC_basecommand,7,8+n,tmp,0);
	int i;
	for (i=0; i < n; i++) {
		retData[i] = tmp[8+i];
	}
	return tmp[5];	// The error code
}

/** Send a command to the VUC with the arguments in a character array of length argLen
	returns: the N data characters from the VUC response will be written to retData
		and the error code character will be returned. Note that you need to know how
		many data characters your command will return!

	The VUC always responds with an 8-byte header
	VUC_MSG_VER_HI VUC_MSG_VER_LO VUC_SW_VER_HI VUC_SW_VER_LO reserved error code ACK_LEN_HI ACK_LEN_LO
	
	This header will NOT be returned. If you need to debug, use VUC() or VUCHex()
 */
char VUC_sendCommandWithArgs(char commandHi, char commandLow, int argLen, char *args, int n, char* retData) {
	// Add the command specifics to our basecommand
	VUC_basecommand[4] = commandHi;
	VUC_basecommand[5] = commandLow;
	char cmdTmp[7+argLen];
	int i = 0;
	while (i < 6) {
		cmdTmp[i] = VUC_basecommand[i];
		i++;
	}
	while (i < 6 + argLen) {
		cmdTmp[i] = args[i-6];
		i++;
	}
	// Add the CRC8
	cmdTmp[i] = GenCRC8(cmdTmp, i);
	
	char tmp[8+n];
	i2c_SnR_v2(VUC_I2C_ADDRESS, cmdTmp, 7+argLen, 8+n, tmp, 0);
	for (i=0; i < n; i++) {
		retData[i] = tmp[8+i];
	}
	return tmp[5];	// The error code
}

/** Returns the stored telemetry pointer. Note that you want to re-request this if
	you choose charOrAscii = 1, as it will not update.
 */
char* VUC_getStoredTelem(int charOrAscii) {
    if (charOrAscii == 0) {
        return vucTelem;
	} else if (charOrAscii == 1) {
	    return asciifiedArray(vucTelem, VUC_TELEM_LEN);
	}
 	return asciifiedArrayNoSpace(vucTelem, VUC_TELEM_LEN);
}

/** Asks VUC for telemetry right now (updating our stored array) and returns the value in
 *	either a char array or asciified string.
 */
char* VUC_getRealtimeTelemetry(int charOrAscii) {
	VUC_sendCommandNoArgs(VUC_BYTE1_COMMAND, VUC_GET_TELEM, VUC_TELEM_LEN, vucTelem);
	// VUC_getStoredTelem handles formatting
	return(VUC_getStoredTelem(charOrAscii));
}

/** Return the two-byte run state indicator, overwriting the input char[] */
void VUC_getRunState(char* dat) {
	VUC_sendCommandNoArgs(VUC_BYTE1_COMMAND, VUC_GET_RUNSTATE, 2, dat);
}

/** Return the two-byte status indicator, overwriting the input char[] */
void VUC_getTime(char* dat) {
	VUC_sendCommandNoArgs(VUC_BYTE1_TIME, VUC_GET_TIME, 6, dat);
}

/** Set the run state to the given value */
void VUC_setRunState(char runState) {
	char stateTmp[2] = {0x00, runState};
	VUC_sendCommandWithArgs(VUC_BYTE1_COMMAND, VUC_SET_RUNSTATE, 2, stateTmp, 2, stateTmp);
}

/** Return the two-byte status indicator, overwriting the input char[] */
void VUC_getStatus(char* dat) {
	VUC_sendCommandNoArgs(VUC_BYTE1_COMMAND, VUC_GET_STATUS, 2, dat);
}

void task_Vand(void) {
	int i;
	int telemetryCycles = 60;	// Number of times through the while() between telemetry pulls
	int telemCount = telemetryCycles;		// Ensures we get telemetry on the first cycle
	// Check for deployed state
	while (!OSReadBinSem(BINSEM_DEPLOYED_P)) {
		OS_Delay(100);
	}
	// Turn on VUC. The VUC will handle its own run state
	OSSignalBinSem(BINSEM_VUC_TURN_ON_P);
	while(1) {
		if (OSTryBinSem(BINSEM_VUC_TURN_ON_P)) {
			csk_io24_low(); 
			csk_io25_low();  //latch
			OS_Delay(10);
			csk_io24_high(); //vcon
			OS_Delay(10);
			csk_io25_high(); 
			OS_Delay(10);
			csk_io25_low(); 
		}
		OS_Delay(50);
		if (OSTryBinSem(BINSEM_VUC_TURN_OFF_P)) {
			// Send the HALT command
			VUC_setRunState(VUC_HALT);
			// Wait ten seconds
			for (i=0; i < 10; i++) {
				OS_Delay(100);
			}
			// Send the OFF command
			VUC_setRunState(VUC_OFF);
			// Set the pins low
			csk_io24_low(); 
			csk_io25_low();
			OS_Delay(10);
			csk_io25_high(); 
			OS_Delay(10);
			csk_io25_low(); 			
		}
		OS_Delay(50);
		// Check whether it's time to get telemetry
		if (telemCount < telemetryCycles) {
			telemCount++;
		} else {
			// Send the command
			VUC_getRealtimeTelemetry(0);
			telemCount = 0;
		}	
	}
} /* task_Vand() */
