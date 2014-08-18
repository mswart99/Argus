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
#include "task_scheduler.h"

// Pumpkin CubeSat Kit headers
#include "csk_io.h"
#include "csk_uart.h"
#include "csk_sd.h"
#include "thin_usr.h"

// Pumpkin Salvo headers
#include "salvo.h"

extern char HeTrans255Str(char* inpt);
extern unsigned int makeHex(char char1, char char2);
extern void CMDS(char a[], char * saveName);


/*typedef struct{
   unsigned long ticker;
   unsigned long time;
   unsigned int limit;
} cmdSchedule;*/

/******************************************************************************
****                                                                       ****
**                                                                           **
task_scheduler()

keeps track of function schedule

**                                                                           **
****                                                                       ****
******************************************************************************/

static int cmdCount = 0;
static char cmdTmp[300]; //Note the below format of the stored command & scheduling information. //Extra long for sanity reasons.
static unsigned long* ticker=(unsigned long*) (cmdTmp); //These locations are statically allocated but are only referencable once cmdTmp has been appropriately set.
static unsigned long* time=((unsigned long*) (cmdTmp))+1;
static unsigned int* limit=(unsigned int*) (((unsigned long*) (cmdTmp))+2);
static char* name=(char*) (((unsigned int*) (((unsigned long*) (cmdTmp))+2))+1);

static char fileNameFormat[]="%04d.sch";

void deleteSchedule(int num) {
	if(num+1<cmdCount) {
		int k;
		for(k=num+1;k<cmdCount;k++){ //This removes this task, and pulls down any tasks above it to fill its spot.
			//Scheduler[k-1]=Scheduler[k];
			sprintf(cmdTmp,fileNameFormat,k);
			F_FILE* file1=f_open(cmdTmp,"r");
			f_seek(file1,0,SEEK_END);
			unsigned int eof=f_tell(file1);
			f_seek(file1,0,SEEK_SET);
			f_read(cmdTmp,1,eof&0xFF,file1);
			f_close(file1);
			sprintf(((char*) (cmdTmp))+257,fileNameFormat,k-1);
			F_FILE* file0=f_open(((char*) (cmdTmp))+257,"w");
			f_write(cmdTmp,1,eof&0xFF,file0);
			f_close(file0);
		}
		cmdCount--;
	}
	else if(cmdCount==1 && num==0) cmdCount--;
	return;
}

void getSchedule(int num, char* a) {
	//Returns the information of the numbered schedule to the array a.
	if(cmdCount>num) {
		sprintf(a,fileNameFormat,num);
		F_FILE* file=f_open(a,"r");
		f_seek(file,0,SEEK_END);
		unsigned int eof=f_tell(file);
		f_seek(file,0,SEEK_SET);
		eof=f_read(a,1,eof&0x00FF,file); //Denana, check return value on fread :)
		a[eof]=0;
		f_close(file);
	}
	else {
		strcpy(a,"none\r\n");
	}
	return;
}

void task_scheduler(void) {
  int i;
  static int j;

  while (1) {
    OS_Delay(100);
	j=0;
	for(j=0;j<cmdCount;j++){ //This is the execution loop for each scheduled task.
		OS_Delay(1);
		sprintf(cmdTmp,fileNameFormat,j);
		F_FILE* aFile=f_open(cmdTmp,"r+");
		f_seek(aFile,0,SEEK_END);
		unsigned int eof=f_tell(aFile);
		f_seek(aFile,0,SEEK_SET);
		f_read(cmdTmp,1,eof&0x00FF,aFile);
		cmdTmp[eof&0x00FF]=0;
		cmdTmp[256]=0;
		f_seek(aFile,0,SEEK_SET);

		(*ticker)--;
		if((*ticker)==0){ //Triggers when a task is ready to be executed. 
			f_close(aFile);
			CMDS(name, 0);
			sprintf(((char*) (cmdTmp))+257,fileNameFormat,j);
			aFile=f_open(((char*) (cmdTmp))+257,"r+");
			f_seek(aFile,0,SEEK_SET);

			(*ticker)=(*time);
			if((*limit)>1){
				(*limit)--;
				f_write(cmdTmp,1,10,aFile);
				f_close(aFile);
			}
			else {
				if((*limit)==1){ //Triggers when a task has depleted its execution limit. 
					f_close(aFile);
					int k;
					for(k=j+1;k<cmdCount;k++){ //This removes this task, and pulls down any tasks above it to fill its spot.
						//Scheduler[k-1]=Scheduler[k];
						sprintf(cmdTmp,fileNameFormat,k);
						F_FILE* file1=f_open(cmdTmp,"r");
						f_seek(file1,0,SEEK_END);
						eof=f_tell(file1);
						f_seek(file1,0,SEEK_SET);
						f_read(cmdTmp,1,eof&0xFF,file1);
						f_close(file1);
						sprintf(((char*) (cmdTmp))+257,fileNameFormat,k-1);
						F_FILE* file0=f_open(((char*) (cmdTmp))+257,"w");
						f_write(cmdTmp,1,eof&0xFF,file0);
						f_close(file0);
					}
					cmdCount--;	
				}//if limit==1
				else { //Limit is 0.
					f_write(cmdTmp,1,10,aFile); //Save the new ticker. 
					f_close(aFile);
				}	
			}
		}//if ticker==0
		else {
			f_write(cmdTmp,1,10,aFile);
			f_close(aFile);
		}
	}//for
	
	if(OSReadMsg(MSG_EDITCMDSCH_P)) { //This is where tasks are added to the schedule.
		char* newCmd=((char*) (OSTryMsg(MSG_EDITCMDSCH_P))); 
		sprintf(cmdTmp,fileNameFormat,cmdCount);
		F_FILE* aFile=f_open(cmdTmp,"w");

		//Reusing cmdTmp here.
		for(i=0;i<4;i++){
			cmdTmp[3-i]=(unsigned char)(makeHex(newCmd[i*2],newCmd[2*i+1])); //Ticker
			cmdTmp[7-i]=cmdTmp[3-i]; //Time
		}
		for(i=0;i<2;i++){
			cmdTmp[9-i]=(unsigned char)(makeHex(newCmd[2*i+8],newCmd[2*i+9])); //Limit
		}
		strcpy(((char*) (cmdTmp))+10,((char*) (newCmd))+12); //name

		f_write(cmdTmp,1,10+strlen(cmdTmp+10),aFile);
		f_close(aFile);
		cmdCount++;
	}//if new command
  } /* while */

} /* task_scheduler() */


