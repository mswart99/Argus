/******************************************************************************
(C) Copyright Pumpkin, Inc. All Rights Reserved.

This file may be distributed under the terms of the License
Agreement provided with this software.

THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE.

$Source: C:\\RCS\\D\\Pumpkin\\CubeSatKit\\Example\\all\\all\\CubeSatKit_Dev_Board\\Test\\Test1\\task_ejection.c,v $
$Author: aek $
$Revision: 3.1 $
$Date: 2009-11-02 00:45:07-08 $

******************************************************************************/
#include "main.h"
#include "task_ejection.h"

// Pumpkin CubeSat Kit headers
#include "csk_io.h"
#include "csk_uart.h"
#include "csk_sd.h"
#include "thin_usr.h"

// Pumpkin Salvo headers
#include "salvo.h"

extern char send_i2c_byte(int data);
extern void i2c_start(void);
extern void i2c_restart(void);
extern void reset_i2c_bus(void);
extern unsigned char i2c_read_ack(void);
extern unsigned char i2c_read_nack(void);

// -----------------------------------------
// -----------------------------------------
// -----------------------------------------
// -----------------------------------------
// -----------------------------------------
// -------------- NOT USED IN V3 AND ABOVE
// -----------------------------------------
// -----------------------------------------
// -----------------------------------------
// -----------------------------------------
// -----------------------------------------


void task_ejection(void) {
  	while(!OSReadBinSem(BINSEM_EJECTED_P)) {
  		OS_Delay(30);
  		unsigned char tmp[6];
  		i2c_start();
		send_i2c_byte((0x68<<1));
		send_i2c_byte(0x02);
		i2c_restart();
		send_i2c_byte((0x68<<1)+1);
		int i;
		for(i=0;i<5;i++) tmp[i]=i2c_read_ack();
		tmp[5]=i2c_read_nack();
		reset_i2c_bus();
		for(i=0;i<100;i++) Nop();
		//Zero out unknown bits.
		tmp[0]=tmp[0]&0b01111111;
		tmp[1]=tmp[1]&0b00111111;
		tmp[3]=tmp[3]&0b00111111;
		tmp[4]=tmp[4]&0b00011111;
		
		//hours >= 12, day of month >=1, month of year>=1, years>=1; that is to say if it has been at least 12 hours.
  		if(tmp[1]>=0x12 || tmp[3]>=1 || tmp[4]>0 || tmp[5]>0) {
			OSSignalBinSem(BINSEM_EJECTED_P);
			csk_uart0_puts("Satellite is Ejected\r\n");
			F_FILE * Deployed_Ejected_SDSave = f_open("DEPEJEC","r+");
			//Record Deployed on SD-Card
			//In file: 1=>just deployed, 2=>just ejected, 3=>both deployed and ejected true.
			if(Deployed_Ejected_SDSave) {
				f_seek(Deployed_Ejected_SDSave,0,SEEK_END);
				unsigned long _eof=f_tell(Deployed_Ejected_SDSave);
				f_seek(Deployed_Ejected_SDSave,0,SEEK_SET);
				if(_eof>0) {
					char SD_SAVESTATE[1];
					f_read(SD_SAVESTATE,1,1,Deployed_Ejected_SDSave);
					f_seek(Deployed_Ejected_SDSave,0,SEEK_SET);
					if(SD_SAVESTATE[0]==1) {
						SD_SAVESTATE[0]=3; //3=>Deployed and Ejected.
						f_write(SD_SAVESTATE,1,1,Deployed_Ejected_SDSave);
					}
					if(SD_SAVESTATE[0]!=3) {
						SD_SAVESTATE[0]=2; //2=>Just Ejected.
						f_write(SD_SAVESTATE,1,1,Deployed_Ejected_SDSave);
					}
				}
				else {
					char SD_SAVESTATE[1];
					SD_SAVESTATE[0]=2; //2=>Just Ejected.
					f_write(SD_SAVESTATE,1,1,Deployed_Ejected_SDSave);
				}
				f_close(Deployed_Ejected_SDSave);
			}
		}
  	}
	while (1) {
		OS_Delay(250);
	} /* while */
} /* task_ejection() */



