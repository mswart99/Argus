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
#include "task_MHXPower.h"
#include "helium.h"

// Pumpkin CubeSat Kit headers 
#include "csk_io.h"
#include "csk_uart.h"
#include "csk_sd.h"
#include "thin_usr.h"

// Pumpkin Salvo headers
#include "salvo.h"


// Microchip PIC24F Peripehral Library
#include <pps.h>
//#include <pwrmgnt.h>
//#include <timer.h>
#include <uart.h>

extern void CMDS (char a[]);
extern void csk_usb_open(void); // defined in csk_usb.c
extern void csk_usb_close(void); // defined in csk_usb.c
extern void HeCkSum(char* buffer, int n);
//extern char HeTrans255Str(char* inpt);

static unsigned char defaultPowerLevel=0x4B;
static unsigned char highPowerLevel=0x87;
//Set all settings (Power, Callsigns, etc)
//static char HEStandardConfig[44]={'H', 'e', 0x10, 0x06, 0x00, 0x22, 0, 0, 0x00, 0, 0x01, 0x01, 0x00, 0x00, 0x19, 0x3A, 0x02, 0x00, 0x32, 0xAC, 0x06, 0x00, 'C', 'O', 'P', 'P', 'E', 'R', 'S', 'L', 'U', 'G', 'N', 'D', 0x05, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0, 0}; //145.945
//static char HEStandardConfig[44]={'H', 'e', 0x10, 0x06, 0x00, 0x22, 0, 0, 0x00, 0, 0x01, 0x01, 0x00, 0x00, 0xEC, 0x39, 0x02, 0x00, 0x32, 0xAC, 0x06, 0x00, 'C', 'O', 'P', 'P', 'E', 'R', 'S', 'L', 'U', 'G', 'N', 'D', 0x05, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0, 0}; //145.900
/* Standard config, by data packet(s)
2 - 'He'
2 - 0x10 0x06 (send configuration to Helium
2 - size of packet to send (0x0022, or 34 characters)
2 - checksums
uint_1 Radio Interface Baud Rate (9600=0x00)
uint_1 Tx Power Amp level (min = 0x00 max = 0xFF)
uint_1 Radio RX RF Baud Rate (9600=0x01)
uint_1 Radio TX RF Baud Rate (9600=0x01)
uint_1 rx_modulation; //(0x00 = GFSK);
uint_1 tx_modulation; //(0x00 = GFSK);
uint_4 //Channel Rx Frequency : the packets are in REVERSE order (68 36 02 00 is 0x00023668 = 145000 kHz)
uint_4 //Channel Tx Frequency : the packets are in REVERSE order (00 AC 06 00 is 0x0006AC00 = 437248 kHz)
unsigned char source[6]; //AX25 Mode Source Call Sign (default NOCALL)
unsigned char destination[6]; //AX25 Mode Destination Call Sign (default CQ)
uint_2 tx_preamble; //AX25 Mode Tx Preamble Byte Length (0x00 = 20 flags)
uint_2 tx_postamble; //AX25 Mode Tx Postamble Byte Length (0x00 = 20 flags)
uint_2 function_config; //Radio Configuration Discrete Behaviors
uint_2 function_config2; //Radio Configuration Discrete Behaviors #2
*/					
static char HEStandardConfig[HE_CONFIG_LEN+10]={'H', 'e', 0x10, 
	SET_TRANSCEIVER_CONFIG, 0x00, 0x22, 0, 0, 
	0, 0, 0x01, 0x01, 0x00, 0x00, 
	0x68, 0x36, 0x02, 0x00, 
	0x00, 0xAC, 0x06, 0x00, 
	CALL0, CALL1, CALL2, CALL3, CALL4, CALL5, 
	GROUND0, GROUND1, GROUND2, GROUND3, GROUND4, GROUND5, 
	0x05, 0x00, 
	0x00, 0x00, 
	0x00, 0x00, 
	0x00, 0x00, 
	0, 0}; //145 even
//static char HEStandardConfig[44]={'H', 'e', 0x10, 0x06, 0x00, 0x22, 0, 0, 0x00, 0, 0x01, 0x01, 0x00, 0x00, 0x68, 0x36, 0x02, 0x00, 0x00, 0xAC, 0x06, 0x00, 'A', 'R', 'G', 'U', 'S', '1', 'S', 'L', 'U', 'G', 'N', 'D', 0x05, 0x00, 0x00, 0x00,	0x40, 0x00, 0x00, 0x00, 0, 0}; //145 even

char* getHeConfig() {
	return HEStandardConfig;
}

void setHeDefaultPowerLevel(unsigned char pl) {
	defaultPowerLevel=pl;
}
void setHeHighPowerLevel(unsigned char pl) {
	highPowerLevel=pl;
}

static unsigned int secsPowerHighAfterContact=20*60;
static unsigned int count;

void commandHeStandardConfig() {
	// We cannot act until the TX line is clear
//	OS_WaitBinSem(BINSEM_CLEAR_TO_SEND_P, OSNO_TIMEOUT);
	int i=0;
	// Format the entire command
	for(i=0;i<HE_CONFIG_LEN+10;i++) {
		csk_uart1_putchar(HEStandardConfig[i]);
	}
}

void commandHeNoBeacons() {
	char HeNoBeacons[11]={0x48,0x65,0x10,0x11,0x00,0x01,0x22,0x74,0x00,0xB8,0x2A};
	int i;
	// Format the entire command
	for(i=0;i<11;i++) {
		csk_uart1_putchar(HeNoBeacons[i]);
	}	
}

void task_MHXPower(void) {
	HEStandardConfig[9]=defaultPowerLevel;
	HeCkSum(HEStandardConfig,6); // Creates checksums on header
	HeCkSum(HEStandardConfig,42); //This will append two bytes to the end.

	OSTryBinSem(BINSEM_HEON_P); //Make sure its 0 (signals that the radio is off).
	OS_Delay(20);
	long i;

	/* We don't wait on BINSEM_CLEAR_TO_SEND_P because
	 * we don't need to hear the answer. Also, no one else should be 
	 * talking to the Helium right now. 
	 * Note that this function will only work if the Helium & CSK are
	 * already on the right UART speed.
	 */
	commandHeNoBeacons();
	OS_Delay(50);
	commandHeStandardConfig();
	OS_Delay(50);
	commandHeStandardConfig();
	OS_Delay(50);
	commandHeStandardConfig();
	// Now, we set the Helium to the correct data rate
	for(i=0; i < 3; i++) {
		csk_uart1_close();
		OS_Delay(50);
	/* ---------- IMPORTANT!!!! ----------------------------------
	 * The default case *must* be the rate that matches HE_BAUD_RATE 
	 * in HeStandardConfig (and you cannot switch on the last value
	 * of i!), or else the radio will be listening at the incorrect rate.
	 *
	 * More savvy C-programmers would be able to better automate this
	 * part of the code, but I got stuck on how all these pieces are
	 * macros of lists. (MAS)
	 */ 
		switch(i) {
			case 0:
				csk_uart1_open(UART_38400_N81_MAIN);
				break;
			case 1:
				csk_uart1_open(UART_19200_N81_MAIN);
				break;
			default:
				csk_uart1_open(UART_9600_N81_MAIN);
		}
		OS_Delay(50);
		commandHeStandardConfig();
		OS_Delay(50);
		commandHeStandardConfig();
		OS_Delay(50);
		commandHeStandardConfig();
	}

	// Wait until the deployment signal is provided
	// This is preferable to WaitBinSem because we don't 
	// want to clear the semaphore value
	while (!OSReadBinSem(BINSEM_DEPLOYED_P)) {
		OS_Delay(100);
	}
	OSSignalBinSem(BINSEM_HEON_P); //Signal that the radio is on!
	csk_uart0_puts("Radio Enabled\r\n");
	
	while(1) {
		count=0;
		while(!OSReadBinSem(BINSEM_RAISEPOWERLEVEL_P)) {
			OS_Delay(250);
			if(count==960) { //40min
				for(i=0;i<100000;i++) Nop();
				// We cannot act until the TX line is clear
				OS_WaitBinSem(BINSEM_CLEAR_TO_SEND_P, OSNO_TIMEOUT);
				for(i=0;i<44;i++) {
					csk_uart1_putchar(HEStandardConfig[i]);
				}
				for(i=0;i<100000;i++) Nop();
				count=0;
			}
			count++;
		}
		OSTryBinSem(BINSEM_RAISEPOWERLEVEL_P);
		char HePowerLevel[11]={'H', 'e', 0x10, 0x20, 0x00, 0x01, 0,0,highPowerLevel};
		HeCkSum(HePowerLevel,6); 
		HeCkSum(HePowerLevel,9); //This will append two bytes to the end.
		for(i=0;i<11;i++) {
			csk_uart1_putchar(HePowerLevel[i]);
		}
		count=0;
		while(count<secsPowerHighAfterContact) {
			if(OSReadBinSem(BINSEM_RAISEPOWERLEVEL_P)) {
				OSTryBinSem(BINSEM_RAISEPOWERLEVEL_P);
				count=0;
			}
			OS_Delay(100);
			count++;
		}
		char HePowerLevel2[11]={'H', 'e', 0x10, 0x20, 0x00, 0x01, 0,0,defaultPowerLevel};
		HeCkSum(HePowerLevel2,6); 
		HeCkSum(HePowerLevel2,9); //This will append two bytes to the end.
		// We cannot act until the TX line is clear
		OS_WaitBinSem(BINSEM_CLEAR_TO_SEND_P, OSNO_TIMEOUT);
		for(i=0;i<11;i++) {
			csk_uart1_putchar(HePowerLevel2[i]);
		}		
	}//While(1)
} /* task_MHXPower() */
