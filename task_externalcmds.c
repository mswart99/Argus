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
#include "task_externalcmds.h"

// Pumpkin CubeSat Kit headers 
#include "csk_io.h"
#include "csk_uart.h"
#define CSK_EFFS_THIN_LIB 1
#include "csk_sd.h"
#include "thin_usr.h"

// Pumpkin Salvo headers
#include "salvo.h"

// Other tasks
#include "task_beacon.h"

extern char * i2c_SnR(int address, char sending[], int numSend, int numRec, char charArrayOfAppropriateSize[], int asciiOrHex);
//extern void i2c_sendz(char sending[]);
//extern char * i2c_readz(int num, char charArrayOfAppropriateSize[], int asciiOrHex);
extern char send_i2c_byte(int data);
extern void i2c_start(void);
extern void i2c_restart(void);
extern void reset_i2c_bus(void);
extern unsigned char i2c_read_ack(void);
extern unsigned char i2c_read_nack(void);
extern void Vand(char a[], int numSent, int numRet);
extern void VandHex(char a[], int numSent, int numRet);
extern void VUC(char a[], int numSend, char returnArray[], int numRet);
extern void VUCHex(char a[], int numSend, char returnArray[], int numRet);
extern char HeTrans255Str(char* inpt);
extern char HeTrans255(char* inpt, int n);
extern void HeCkSum(char* buffer, int n);
extern char HeTrans255(char* inpt, int n);
extern unsigned long long getMissionClock();
extern void deleteSchedule(int num);
extern void getSchedule(int num, char* a);
extern void setHeSaveData3(char* a);
extern void setHeDefaultPowerLevel(unsigned char pl);
extern void setHeHighPowerLevel(unsigned char pl);
extern void setBeacon2_5SecInterval(unsigned char num);
extern void commandHeStandardConfig();
extern char* getHeConfig();
extern void commandHeNoBeacons();
extern void setBeaconFrameIntervals(unsigned int *nums);

unsigned int makeHex(char char1, char char2) {
	int total=0;
	if (char1==49) total=total+1*16;
	if (char1==50) total=total+2*16;
	if (char1==51) total=total+3*16;
	if (char1==52) total=total+4*16;
	if (char1==53) total=total+5*16;
	if (char1==54) total=total+6*16;
	if (char1==55) total=total+7*16;
	if (char1==56) total=total+8*16;
	if (char1==57) total=total+9*16;
	if (char1==65) total=total+10*16;
	if (char1==66) total=total+11*16;
	if (char1==67) total=total+12*16;
	if (char1==68) total=total+13*16;
	if (char1==69) total=total+14*16;
	if (char1==70) total=total+15*16;
	if (char2==49) total=total+1;
	if (char2==50) total=total+2;
	if (char2==51) total=total+3;
	if (char2==52) total=total+4;
	if (char2==53) total=total+5;
	if (char2==54) total=total+6;
	if (char2==55) total=total+7;
	if (char2==56) total=total+8;
	if (char2==57) total=total+9;
	if (char2==65) total=total+10;
	if (char2==66) total=total+11;
	if (char2==67) total=total+12;
	if (char2==68) total=total+13;
	if (char2==69) total=total+14;
	if (char2==70) total=total+15;
	return total;
}

void BroadcastOrSave(char a[], char * ptr){
	if (!ptr) {
		HeTrans255Str(a);
	}
	else {
		F_FILE * file = f_open(ptr, "a");
		f_write(a, 1, strlen(a), file);
		f_close(file);
	}
}
void BroadcastOrSaveN(char a[], char * ptr, int num){
	if (!ptr) {
		HeTrans255(a,num);
	}
	else {
		F_FILE * file = f_open(ptr, "a");
		f_write(a, 1, num, file);
		f_close(file);
	}
}

static int ZEROCLOCKINT=0;
static int IRONMANINT=0;

void CMDS(char a[], char * saveName) {
	OSSignalBinSem(BINSEM_RAISEPOWERLEVEL_P); 
	csk_uart0_puts("task_externalcmds:\t");
	csk_uart0_puts(a);
	csk_uart0_puts("\r\n");
	char tmp[400]; 
	int I;
	for(I=0;I<1000;I++) Nop();
	if (a[0]=='\r' || a[0]=='\n' || a[0]==0) { 
		return;
	}
	if (a[0]=='i' && a[1]=='2' && a[2]=='c') { //if (a begins with i2c) !!!
		/*Expects i2caaLLxx...
		i2c=Command block (gonna speak i2c).
		aa=Address of slave device you are going to talk to.
		LL=Number of bytes you want back, in ASCII encoded HEX.
		xx...=The data you are sending, in ASCII encoded HEX, up to 256 bytes (xx=one byte)
		*/
		if (strlen(a)>6)  {
			int add;
			int numRec;				
			if ((strlen(a)%2) != 1) {
				a[strlen(a)-1]=0;
			}
			add=makeHex(a[3],a[4]);
			numRec=makeHex(a[5],a[6]);
			int i;
			int newSizeOfa=0;
			int stra=strlen(a);
			for (i=7;i<stra;i=i+2) {
				a[newSizeOfa]=makeHex(a[i],a[i+1]);
				newSizeOfa++;
			}
			/*for (i=strlen(a)-newSizeOfa-1;i>1;i--) {
				a[strlen(a)-1]=0;
			}*/
			if (numRec>251) numRec=251;
			i2c_SnR(add,a,newSizeOfa,numRec,a,0); //a becomes this one's return.
			//sprintf(tmp,"i2c:%s",a);
			strcpy(tmp,"i2c:");
			for(i=0;i<numRec;i++) tmp[i+4]=a[i];
			for(i=0;i<numRec+4;i++) csk_uart0_putchar(tmp[i]);
			//csk_uart0_puts(tmp);
			BroadcastOrSaveN(tmp, saveName, numRec+4);
		}
		return;
	}//i2c

	if (a[0]=='R' && a[1]=='H' && a[2]=='E') { // if (a begins with RHE) !!!
		/*Resets the Helium radio
		*/
		
		char HeOut[50]={'H', 'e', 0x10, 0x02, 0, 0x01, 0,0};
		int i;
		HeCkSum(HeOut,6); // Formats the header checksum
		HeCkSum(HeOut,8); // Formats the payload checksum, and appends two bytes to the end.
	
		// We cannot act until the TX line is clear
		OS_WaitBinSem(BINSEM_CLEAR_TO_SEND_P, OSNO_TIMEOUT);
		for(i=0;i<10;i++) {
			csk_uart1_putchar(HeOut[i]);
		}
		return;
	}//RHE

	if(a[0] == 'R' && a[1] == 'I' && a[2] == 'M' && a[3] == 'L') {// // if a (beings with RIML!!!)
		/* Expects RIML
		*/
		IRONMANINT=0;
		return;
	}

	if(a[0]=='I' && a[1]=='R' && a[2]=='O' && a[3]=='N' && a[4]=='M' && a[5]=='A' && a[6]=='N' && a[7]=='L' && a[8]=='i' && a[9]=='v' && a[10]=='e' && a[11]=='s' && a[12]=='1') {// // if a (beings with IRONMANLives1!!!)
		/* Expects IRONMANLives1
		*/
	}

	if(a[0]=='I' && a[1]=='R' && a[2]=='O' && a[3]=='N' && a[4]=='M' && a[5]=='A' && a[6]=='N' 
		&& a[7]=='L' && a[8]=='i' && a[9]=='v' && a[10]=='e' && a[11]=='s') {
		// Switch on the next character
		if ((a[12] == '0') && (IRONMANINT==0)) {
			IRONMANINT=1;
			return;
		} else if ((a[12] = '1') && (IRONMANINT==1))  {
			IRONMANINT=2;
			return;
		} else if ((a[12] = '2')&& (IRONMANINT==2)) {
			while(1) {} //Cause a watchdog reset!
			return;
		}
		// If we get here, we haven't followed the sequence
		sprintf(tmp,"IRONMANINT:%d",IRONMANINT);
		csk_uart0_puts(tmp);
		HeTrans255Str(tmp);
		CMDS("RIML", 0);
		return;
	}

	if (a[0]=='V' && a[1]=='D' && a[2]=='L' && a[3]=='B') { //if (a begins with VDLB) !!!
		/*Gets the number of bytes needed to be downlinked from Independence.
		*/
		unsigned char DLbytes[4];
		unsigned long long totalbytes;
		unsigned long long DLB3;
		unsigned long long DLB2;
		unsigned long long DLB1;
		unsigned long long DLB0;

		DLbytes[0]=2;
		DLbytes[1]=0;
//		VUC(DLbytes,2, DLbytes, 4);
		DLB3=DLbytes[3];
		DLB2=DLbytes[2];
		DLB1=DLbytes[1];
		DLB0=DLbytes[0];
		totalbytes=DLB3+DLB2*256+DLB1*256*256+DLB0*256*256*256;
		sprintf(tmp,"task_externalcmds:\t%llu bytes\r\n",totalbytes);
		csk_uart0_puts(tmp);
		BroadcastOrSave(tmp, saveName);
		return;
	}//VDLB

	if (a[0]=='V' && a[1]=='A' && a[2]=='N' && a[3]=='D') { //if (a begins with VAND) !!!
		/*Expects VANDLLxx...
		VAND=Command block (gonna speak to Vandy).
		LL=Number of bytes you want back, in ASCII encoded HEX.
		xx...=The data you are sending, in ASCII encoded HEX,up to 256 bytes (xx=one byte)
		*/
		if (strlen(a)>6)  {
			int numRec=makeHex(a[4],a[5]);
			int i;	
			if ((strlen(a)%2) != 0) {
				a[strlen(a)-1]=0;
			}
			int newSizeOfa=-1;
			for (i=6;i<strlen(a);i=i+2) {
				a[newSizeOfa+1]=makeHex(a[i],a[i+1]);
				newSizeOfa++;
			}
			for (i=strlen(a)-newSizeOfa;i>1;i--) {
				a[strlen(a)-1]=0;
			}

			if (numRec>78) numRec=78;
			VandHex(a,newSizeOfa+1,numRec);
			sprintf(tmp,"task_externalcmds:\t%s\r\n",a);
			csk_uart0_puts(tmp);
			BroadcastOrSave(tmp, saveName);
		}
		return;
	}//VAND

	if (a[0]=='V' && a[1]=='U' && a[2]=='C') { //if (a begins with VUC) !!!
		/*Expects VUCLLxx...
		VUC=command block (gonna speak to the VUC).
		LL=Number of bytes you want back, in ASCII encoded HEX.
		xx...=The data you are sending, in ASCII encoded HEX,up to 256 bytes (xx=one byte)
		*/
		if (strlen(a)>5)  {
			int numRec=makeHex(a[3],a[4]);
			int i;	
			if ((strlen(a)%2) != 1) {
				a[strlen(a)-1]=0;
			}
			int newSizeOfa=0;
			int STRLEN=strlen(a);
			for (i=5;i<STRLEN;i=i+2) {
				a[newSizeOfa]=makeHex(a[i],a[i+1]);
				newSizeOfa++;
			}
			if (numRec>78) numRec=78;
			VUCHex(a,newSizeOfa,a,numRec);
			sprintf(tmp,"task_externalcmds:\t%s\r\n",a);
			csk_uart0_puts(tmp);
			BroadcastOrSave(tmp, saveName);
		}
		return;
	}//VUC

	if (a[0]=='V' && a[1]=='O' && a[2]=='N') { //if (a begins with VON) !!!
		/*Expects VON 
		VON=Command Block (turns on Independence and Commodore)
		*/
		OSSignalBinSem(BINSEM_VUC_TURN_ON_P);
		strcpy(tmp, "VON Sequence Initiated");
		BroadcastOrSave(tmp, saveName);
		return;
	}//VON

	if (a[0]=='V' && a[1]=='O' && a[2]=='F' && a[3]=='F') { //if (a begins with VOFF) !!!
		/*Expects VOFF 
		VOFF=Command Block (turns off Independence and Commodore)
		*/
		OSSignalBinSem(BINSEM_VUC_TURN_OFF_P);
		strcpy(tmp, "VOFF Sequence Initiated");
		BroadcastOrSave(tmp, saveName);
		return;
	}//VOFF

	if (a[0]=='S' && a[1]=='C' && a[2]=='C') { //if (a begins with SCC) !!!
		/*Expects SCCTTTTTTTTllllxx...
		SCC=command block (gonna schedual a command .)
		TTTTTTTT= 4 bytes denoting time between commands in seconds
		llll=2 byte command limit.  0000 if no limit
		xx...=the command being scheduled in ascii
		*/
		if (strlen(a) > 17){
			char message[strlen(a)-2];
			int i=0;
			//copy stuff from a to b.
			for (i=0;i<strlen(a)-3;i++){
				message[i]=a[i+3];
			}
			message[strlen(a) - 3] = 0;
			if (strlen(message) >= 15) OSSignalMsg(MSG_EDITCMDSCH_P,(OStypeMsgP) (message));
			return;
		}
	}//SCC
	
	// SD commands deal with reading/writing to the SD card
	if (a[0] == 'S' && a[1] == 'D') {
		if (a[2] == 'S') { // SDS gets file size
			/* Expects SDSname, where name is file name (max 8 chars)
			 */
			F_FILE* file=f_open(((char*) (a))+3,"r"); //the w option discards any file contents, but does leave the file entry.
			if(file) {
				f_seek(file, 0L, SEEK_END);
				unsigned long sz = f_tell(file);
				sprintf(tmp, "%s is %lu bytes\r\n", (char*) (a) + 3, sz);
				BroadcastOrSave(tmp, saveName);
				f_close(file);
			}
			else {					
				sprintf(tmp, "File not found: %s\r\n", (char*) (a) + 3);
				BroadcastOrSave(tmp, saveName);
			}
			return;
		}	
		if (a[2] == 'R') { // SDR downloads data from a file
			/* Expects SDRFFFFFFFFLLLLLLLLname...
			SDR = Command block
			FFFFFFFF = Offset in file, in ASCII encoded HEX
			LLLLLLLL = Length of bytes to be read, in ASCII encoded HEX
			name... = file name, max of 8 characters, is case sensitive
			*/
			if(strlen(a)>20){
				a[27]=0;
				char message[strlen(a)+1]; //+1 for the null terminator.
				strcpy(message,a);
				OSSignalMsg(MSG_SDR_P,((OStypeMsgP) (message)));
				return;
			}
			// If it doesn't fit; fall down to error
		} else if (a[2] == 'F') { // SDF Downloads a standard file
			// switch on the value of a[3]
			char message[27];
			char fName[7];
			// Default is no file
			fName[0] = 0;
			if (a[3] == 'B') { // Download the entire beacon file
				strcpy(fName, "BEACON");
			} else if (a[3] == 'R') { // Download the entire RSSI file
				strcpy(fName, "RSSI");
			} else if (a[3] == 'D') { // Download the entire DEPEJEC file
				strcpy(fName, "DEPEJEC");
			} else if (a[3] == 'C') { // Download the entire CONFIG file
				strcpy(fName, "CONFIG");
			} else if (a[3] == 'N') { // Download the entire CONFIG file
				strcpy(fName, "names");
			}
			sprintf(message,"SDR0000000000100000%s", fName);
			OSSignalMsg(MSG_SDR_P,((OStypeMsgP) (message)));
			return;
		} else if (a[2] == 'W') { // SDW overwrites data to a file
			/*Expects SDWLn...cmd...
			SDW = Command block
			L = Number of bytes of the file name
			n... = name of the file
			cmd... = command to execute
			*/
			char nameLength = makeHex('0',a[3]);
			char name[9];
			int i;
			for (i=0;(i<nameLength && i<8);i++){
				name[i] = a[4+i];
			}
			name[i] = 0;
			int cmdLength = strlen(a) - 4 - nameLength;
			for (i = 0; i < cmdLength; i++){
				a[i] = a[4+nameLength+i];
			}
			a[cmdLength] = 0;
			CMDS(a, name);
			return;
		} else if (a[2] == 'A') { // SDA appends data to file
			/*Expects SDALn...data...
			SDW = Command block
			L = Number of bytes of the file name
			n... = name of the file
			data...=ascii data to append to the file
			*/
			char nameLength = makeHex('0',a[3]);
			char name[9];
			int i;
			for (i=0;(i<nameLength && i<8);i++){
				name[i] = a[4+i];
			}
			name[i] = 0;

			int cmdLength = strlen(a) - 4 - nameLength;
			for (i = 0; i < cmdLength; i++){
				a[i] = a[4+nameLength+i];
			}
			a[cmdLength] = 0;
		
			BroadcastOrSave(a, name);
			return;
		} else if (a[2] == 'd' && a[3] == 'e' && a[4] == 'l' && a[5] == 'e' && a[6] == 't' && a[7] == 'e') {// // if a (beings with SDdelete!!!)
			/* Expects SDdeletename...
			SDdelete = Command block
			name = file name, max of 8 bytes
			*/
			if(strlen(a)>8) {
				/*if(remove(((char*) (a))+8)) csk_uart0_puts("File Removed\r\n");
				else csk_uart0_puts("File Not found\r\n");*/
				F_FILE* file=f_open(((char*) (a))+8,"w"); //the w option discards any file contents, but does leave the file entry.
				if(file) {
					BroadcastOrSave("File Overwritten\r\n", saveName);
					f_close(file);
				}
				else {
					BroadcastOrSave("File Not found\r\n", saveName);
				}
			}
			return;
		}
		// Commmand not recogiaed; let it fall to the end
	} // End SD family of commands

	if(a[0] == 'R' && a[1] == 'Z' && a[2] == 'C' && a[3] == 'L') {// // if a (beings with RZCL!!!)
		/* Expects RZCL
		*/
		ZEROCLOCKINT=0;
		return;
	}

	if(a[0]=='Z' && a[1]=='E' && a[2]=='R' && a[3]=='O' 
		&& a[4]=='C' && a[5]=='l' && a[6]=='o' && a[7]=='c' && a[8]=='k') {
		// Switch on the next character
		if ((a[9]=='0') && (ZEROCLOCKINT==0)) { 
			ZEROCLOCKINT=1;
			return;
		} else if ((a[9]=='1') && (ZEROCLOCKINT==1)) { 
			ZEROCLOCKINT=2;
			return;
		} else if ((a[9]=='2') && (ZEROCLOCKINT==2)) { 
			// Reset all to initial conditions
			long i;
			CMDS("ION",0);
			//Set up & zero out RTC. Resets to State 0.
			unsigned char  RTCTemp;
			for(RTCTemp=0;RTCTemp<8;RTCTemp++) {
				i2c_start();
				send_i2c_byte((0x68<<1));
				send_i2c_byte(RTCTemp);
				send_i2c_byte(0x00);
				reset_i2c_bus();
				for(i=0;i<1000;i++) Nop();
			}
			i2c_start();
			send_i2c_byte((0x68<<1));
			send_i2c_byte(0x0C);
			send_i2c_byte(0x3F);
			reset_i2c_bus();
			for(i=0;i<1000;i++) Nop();

			OSTryBinSem(BINSEM_DEPLOYED_P);
			OSTryBinSem(BINSEM_EJECTED_P);

			//In file: 1=>just deployed, 2=>just ejected, 3=>both deployed and ejected true, 0=>Neither are true.
			F_FILE * Deployed_Ejected_SDSave = f_open("DEPEJEC","w");
			if(Deployed_Ejected_SDSave) {
				char tmp[1]={0};
				f_write(tmp,1,1,Deployed_Ejected_SDSave);
				f_close(Deployed_Ejected_SDSave);
			}
			Deployed_Ejected_SDSave = f_open("BEACON","w");
			f_close(Deployed_Ejected_SDSave);
			Deployed_Ejected_SDSave = f_open("RSSI","w");
			f_close(Deployed_Ejected_SDSave);
			for(i=0;i<500000;i++) Nop();
//			csk_uart2_putchar(0xAB);
			for(i=0;i<500000;i++) Nop();
			//IRONMANINT=2;
			//CMDS("IRONMANLives2",0);
			return;
		}
		// If we get here, we're off-sequence. Reset the counter
		sprintf(tmp,"ZEROCLOCKINT:%d",ZEROCLOCKINT);
		csk_uart0_puts(tmp);
		HeTrans255Str(tmp);
		CMDS("RZCL",0);
		return;
	}

	if(a[0]=='B' && a[1]=='U' && a[2]=='R' && a[3]=='N') { // if a (beings with BURN!!!)
		OSSignalBinSem(BINSEM_BURNCIRCUIT_P);
		return;
	}

	if(a[0]=='H' && a[1]=='P' && a[2]=='A' && a[3]=='S') { // if a (beings with HPAS!!!)
		/* Expects HPASxx...
		HPAS = Command block
		xx...=The data you are sending, in ASCII encoded HEX,up to 256 bytes (xx=one byte)
		*/
		long i;
		int newSizeOfa=0;
		if ((strlen(a)%2) != 0) {
			a[strlen(a)-1]=0;
		}
		int strlena=strlen(a);
		for (i=4;i<strlena;i=i+2) {
			a[newSizeOfa]=makeHex(a[i],a[i+1]);
			newSizeOfa++;
		}
		for(i=0;i<100000;i++) Nop();
		// We cannot act until the TX line is clear
		OS_WaitBinSem(BINSEM_CLEAR_TO_SEND_P, OSNO_TIMEOUT);
		for(i=0;i<newSizeOfa;i++) csk_uart1_putchar(a[i]);
		for(i=0;i<100000;i++) Nop();
		return;
	}

	if(a[0]=='T' && a[1]=='I' && a[2]=='M' && a[3]=='M') { // if a (beings with TIMM!!!)
		sprintf(a,"task_externalcmds:\t%llu\r\n",getMissionClock());
		csk_uart0_puts(a);
		BroadcastOrSave(a, saveName);
		return;
	}

	if (a[0]=='S' && a[1]=='C' && a[2]=='T') {  // if a (beings with STC!!!)
		/*Set Clock Time
		Expects SCTtttttttt
		SCT=Command block
		tttttttt=UNIX time in hex
		Returns nothing.
		*/
		//if(strlen(a)<11) return;
		unsigned char b[7];
		b[0]=makeHex(a[3],a[4]);
		b[1]=makeHex(a[5],a[6]);
		b[2]=makeHex(a[7],a[8]);
		b[3]=makeHex(a[9],a[10]);
		b[4]=0;

		char VUCTemp[6]={0x01,0x00,b[0],b[1],b[2],b[3]};
//		VUC(VUCTemp,6,VUCTemp,0); //Send time to VUC.
		//unsigned long* uPoint;
		//uPoint=b;
		//unsigned long uTime=(*b);
		
		unsigned long uTime = b[0];
		uTime = (uTime<<8) + b[1];
		uTime = (uTime<<8) + b[2];
		uTime = (uTime<<8) + b[3];

		unsigned long now = 1343779200;  // "1343779200 " is Unix time for 12:00 AM, August 1st, 2012
		
		if(uTime>now){
        	uTime = uTime-now ;
		}
		else{
			char TMD[50];
			sprintf(TMD,"Task_externalcmds\t\t FAILURE\r\n");
			BroadcastOrSave(TMD, saveName);
			return;
		}
		unsigned long month = 8;	
		unsigned long year = ((((uTime/60)/60)/24)/365)+12;
		uTime=uTime-(year-12)*60*60*24*365;
		unsigned long day = (((uTime/60)/60)/24)+1;
		uTime=(uTime-(day-1)*60*60*24);
        unsigned long hour = (uTime/(60*60));
        uTime = uTime - 60*hour*60;
		unsigned long minute = (uTime/(60));
		uTime=uTime-minute*(60);
		unsigned long second = uTime;

        day = day - (year-12)/4;
        if (day<1)
            {day=day+365;}

		if (day<32) 
			{month = 8;}
        else if (day<62)
			{month = 9; day = day-31;} 
        else if (day<93) 
			{month = 10; day = day-61;} 
        else if (day<123) 
			{month = 11; day = day-92;} 
        else if (day<153) 
			{month = 12; day = day-122;} 
        else if (day<185) 
			{month = 1; year=year+1; day = day-153;} 
        else if (day<213) 
			{month = 2; year=year+1; day = day-184;} 
        else if (day<244) 
			{month = 3; year=year+1; day = day-212;
            if (year%4==0)
                {day=day-1;}
            if (day==0)
                {month = 2;
                day = 29;}
			}
        else if (day<274) 
			{month = 4; year=year+1; day = day-243; 
            if (year%4==0)
                {day=day-1;}
            if (day==0)
                {month = 3;
                day = 31;}
			}
        else if (day<305) 
			{month = 5; year=year+1; day = day-273; 
            if (year%4==0)
                {day=day-1;}
            if (day==0)
				{month = 4;
                day = 30;}
   			}         
        else if (day<335) 
			{month = 6; year=year+1; day = day-304; 
            if (year%4==0)
                {day=day-1;}
            if (day==0)
                {month = 5;
                day = 31;}
			}
        else 
			{month = 7; year=year+1; day = day-334;
            if (year%4==0)
                {day=day-1;}
            if (day==0)
                {month = 6;
                day = 30;}
			}

			unsigned char sec = second%10;
			unsigned char tenSec = second/10;
			unsigned char min = minute%10;
			unsigned char tenMin = minute/10;
			unsigned char hor = hour%10;
			unsigned char tenHor = hour/10;
			unsigned char da = day%10;
			unsigned char tenDa = day/10;
			unsigned char mon = month%10;
			unsigned char tenMon = month/10;
			unsigned char yer = year%10;
			unsigned char tenYer = year/10;
			b[0]=sec+(tenSec<<4);
			b[1]=min+(tenMin<<4);
			b[2]=hor+(tenHor<<4);
			b[4]=da+(tenDa<<4);
			b[5]=mon+(tenMon<<4);
			b[6]=yer+(tenYer<<4);
		
		b[0]=b[0]&0b01111111;	 //Stop bit off setting.
		
		// format seconds minutes hours days months years and save as character array a[0-6]
		unsigned char  RTCTemp;
		for(RTCTemp=1;RTCTemp<8;RTCTemp++) {
			if(RTCTemp!=4){
				i2c_start();
				send_i2c_byte((0x68<<1));
				send_i2c_byte(RTCTemp);
				send_i2c_byte(b[RTCTemp-1]);
				reset_i2c_bus();
				int i = 0;
				for(i=0;i<1000;i++) Nop();
			}
		}
		i2c_start(); //This turns off the halt bit.
		send_i2c_byte((0x68<<1));
		send_i2c_byte(0x0C);
		send_i2c_byte(0x3F);
		reset_i2c_bus();
		int i = 0;
		for(i=0;i<1000;i++) Nop();
		return;
    }//SCT

	if (a[0]=='T' && a[1]=='I' && a[2]=='M' && a[3]=='R') { //TIMR
		unsigned char outpt[8];
		long i;

		//Grab time registers from RTC.
		i2c_start();
		send_i2c_byte((0x68<<1));
		send_i2c_byte(0x00);
		i2c_restart();
		send_i2c_byte((0x68<<1)+1);
		for(i=0;i<7;i++) outpt[i]=i2c_read_ack();
		outpt[7]=i2c_read_nack();
		reset_i2c_bus();
		for(i=0;i<1000;i++) Nop();

		//Zero out unknown bits.
		outpt[1]=outpt[1]&0b01111111;
		outpt[2]=outpt[2]&0b01111111;
		outpt[3]=outpt[3]&0b00111111;
		//outpt[4] will be ignored.
		outpt[5]=outpt[5]&0b00111111;
		outpt[6]=outpt[6]&0b00011111;

		sprintf(a,"task_externalcmds:\t%02X:%02X:%02X.%02X %02X/%02X/20%02X\r\n",outpt[3],outpt[2],outpt[1],outpt[0],outpt[5],outpt[6],outpt[7]);
		csk_uart0_puts(a);
		BroadcastOrSave(a, saveName);
		
		return;
    }//TIM
	
	if (a[0]=='D' && a[1]=='E' && a[2]=='L' && a[3]=='T') { //DELT
		//Deletes a scheduled task.
		//Expects: DELTxxxx
		//DELT=command Block.
		//xxxx=number of task to delete (0+).
		if(strlen(a)==8) {
			int num;
			unsigned char* intPtr=((unsigned char*) (&num));
			(*intPtr)=makeHex(a[6],a[7]);
			intPtr++;
			(*intPtr)=makeHex(a[4],a[5]);
			deleteSchedule(num);
			return;
		}
    }//DELT

	if (a[0]=='G' && a[1]=='E' && a[2]=='T' && a[3]=='T') { //GETT
		//Returns all information about a scheduled task.
		//Expects: GETTxxxx
		//GETT=command Block.
		//xxxx=number of task to return (0+).
		if(strlen(a)==8) {
			int num;
			unsigned char* intPtr=((unsigned char*) (&num));
			(*intPtr)=makeHex(a[6],a[7]);
			intPtr++;
			(*intPtr)=makeHex(a[4],a[5]);
			getSchedule(num,a);
			if(a[0]=='n' && a[1]=='o' && a[2]=='n' && a[3]=='e') {
				csk_uart0_puts(a);
				HeTrans255Str(a);
			}
			else {
				int i;
				for(i=0;i<10+strlen(((char*) (a))+10);i++) csk_uart0_putchar(a[i]);
				HeTrans255(a,10+strlen(((char*) (a))+10));
			}
			return;
		}
    }//GETT	

	if (a[0]=='H' && a[1]=='E' && a[2]=='S' && a[3]=='D') { //HESD
		//
		//Expects: HESDxx
		//xx is the He20xx command data to save to the SD-Card
		//Must be used in conjunction with SDW, ie: SDWLn...HESDxx
		if(strlen(a)>=6 && saveName) {
			a[0]=makeHex(a[4],a[5]);
			strcpy(((char*) (a))+1,saveName);
			setHeSaveData3(a);
			return;
		}
    }//HESD

	if (a[0]=='H' && a[1]=='P' && a[2]=='L' && a[3]=='H') { //HPLH
		//HPLHxx
		//xx=high-state power-level for the He Radio
		setHeHighPowerLevel(makeHex(a[4],a[5]));
		return;
    }//HPLH

	if (a[0]=='H' && a[1]=='P' && a[2]=='L' && a[3]=='L') { //HPLL
		//HPLHxx
		//xx=high-state power-level for the He Radio
		setHeDefaultPowerLevel(makeHex(a[4],a[5]));
		return;
    }//HPLL

	if (a[0]=='B' && a[1]=='E' && a[2]=='P') { //BEP
		//BEPxxyyzz
		//xx= time in seconds between beacon frames
		//yy= number of frames between sending frame 1
		//zz= number of frames between sending frame 2

		// Error check: we have enough parameters
		if (strlen(a) >= 5 + 2*BEACON_NUM_FRAMES) {
			unsigned int bintervals[BEACON_NUM_FRAMES+1];
			int i;
			for (i=0; i < BEACON_NUM_FRAMES+1; i++) {
				bintervals[i] = makeHex(a[3+2*i], a[4+2*i]);
			}
			setBeaconFrameIntervals(bintervals);
			return;
		} // If we fail, then let it drop to the error message
    }//BEP

	if (a[0]=='H' && a[1]=='E' && a[2]=='S' && a[3]=='C') { //HESC
		//HESC
		commandHeStandardConfig();	// Found in task_MHXPower
		return;
    }//HESC

	if (a[0]=='H' && a[1]=='E' && a[2]=='B' && a[3]=='O') { //HEBO
		//HEBO
		commandHeNoBeacons();
		return;
    }//HEBO

	if (a[0]=='H' && a[1]=='E' && a[2]=='C' && a[3]=='P') { //HECP
		//HECP(88bytes)
		//(88bytes)=the new standard configuration packet.
		if(strlen(a)==92) {
			char* HeStandardConfig=getHeConfig();
			int i;
			for(i=0;i<4;i++) HeStandardConfig[14+i]=makeHex(a[4+2*i],a[5+2*i]);
			return;
		}
    }//HECP

	if (a[0]=='B' && a[1]=='N' && a[2]=='O' && a[3]=='W') { //BNOW
		// Send a beacon right now (set the semaphore high)
		OSSignalBinSem(BINSEM_SEND_BEACON_P);	
		return;
    }//BNOW

	if (a[0]=='H' && a[1]=='B' && a[2]=='I' && a[3]=='N') { //HBIN
		// Set the Helium binary semaphore high (enable Tx)
		OSSignalBinSem(BINSEM_HEON_P);
		return;
    }//HBIN
	if (a[0]=='E' && a[1]=='J' && a[2]=='E' && a[3]=='C') { //EJEC
		// Set the Ejection binary semaphore to high
		OSSignalBinSem(BINSEM_EJECTED_P);
		return;
    }//EJEC
	if (a[0]=='E' && a[1]=='C' && a[2]=='H' && a[3]=='O') { //ECHO
		HeTrans255Str(a);
		return;
    }//ECHO

	//At this point, no command has been recognized, as it would have returned if it had been.
	csk_uart0_puts("task_externalcmds:\tCommand NOT Recognized\r\n");
	BroadcastOrSave("task_externalcmds:\tCommand NOT Recognized\r\n", saveName);
//	HeTrans255Str(a);
}

void task_externalcmds(void) {
  	char a[256];
	static unsigned int i=0;
	while(1) {
		OS_Delay(250);
		int waitTmp=1; //ENTER: Will wait until something is received, and store it in a.
		while(waitTmp) {
			OS_Delay(20);
			waitTmp=1;
			//strcpy(a,"");
			i=0;
			while(csk_uart0_count() && i<256) {
				//sprintf(a,"%s%c",a,csk_uart0_getchar());
				a[i]=csk_uart0_getchar();
				waitTmp=0;
				i++;
			}
			a[i]=0;
		}  //END: Will wait until something is received, and store it in a.
		CMDS(a, 0);
	}
} /* task_externalcmds() */


