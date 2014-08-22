/******************************************************************************
(C) Copyright Pumpkin, Inc. All Rights Reserved.

This file may be distributed under the terms of the License
Agreement provided with this software.

THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE.

$Source: C:\\RCS\\D\\Pumpkin\\CubeSatKit\\Example\\all\\all\\CubeSatKit_Dev_Board\\Test\\Test1\\task_SDR.c,v $
$Author: aek $
$Revision: 3.1 $
$Date: 2009-11-02 00:45:07-08 $

******************************************************************************/
#include "main.h"
#include "task_SDR.h"

// Pumpkin CubeSat Kit headers
#include "csk_io.h"
#include "csk_uart.h"
#include "csk_sd.h"
#include "thin_usr.h"

// Pumpkin Salvo headers
#include "salvo.h"

extern char HeTrans255(char* inpt, int n);
extern unsigned int makeHex(char char1, char char2);

void task_SDR(void) {

  while (1) {
    OS_Delay(250);
	if(OSReadMsg(MSG_SDR_P)) {
		static char* a;
		a=((char*) (OSTryMsg(MSG_SDR_P)));
		static unsigned long offset;
		static unsigned long len;
		int i;
		for(i=0;i<4;i++){
			((unsigned char*) (&offset))[3-i] = makeHex(a[2*i+3],a[2*i+4]);
			((unsigned char*) (&len))[3-i] = makeHex(a[2*i+11],a[2*i+12]);
		}
		char* Filename=((char*)(a))+19;
		static F_FILE * file;
		file=f_open(Filename,"r");
		f_seek(file,0,SEEK_END);
		static unsigned long eof;
		eof=f_tell(file);
		f_seek(file,0,SEEK_SET);
		if(offset>eof) {offset=0; len=0;}
		else {
			if(offset+len>eof) len=eof-offset;
		}
		if(file && !f_seek(file,offset,0)) { //f_seek returns zero on success.
			static char data[255];
			for(i=0;i<255;i++) data[i]=0;
			while(len>0) {
				if(len<=255) {
					f_read(data,1,len,file);
					HeTrans255(data,len);
					for(i=0;i<len;i++) csk_uart0_putchar(data[i]);
					len=0;
				}
				else {
					f_read(data,1,255,file);
					HeTrans255(data,255);
					for(i=0;i<255;i++) csk_uart0_putchar(data[i]);
					len=len-255;
				}
				OS_Delay(1);
			}
			f_close(file);
		}
	}
  } /* while */
} /* task_SDR() */


