/******************************************************************************
(C) Copyright Pumpkin, Inc. All Rights Reserved.

This file may be distributed under the terms of the License
Agreement provided with this software.

THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE.

$Source: C:\\RCS\\D\\Pumpkin\\CubeSatKit\\Example\\all\\all\\CubeSatKit_Dev_Board\\Test\\Test1\\task_beacon.c,v $
$Author: Nathan Bossart, Wesley Gardner and Michael Swartwout $
$Revision: 7.0 $
$Date: 2014-08-14  $

******************************************************************************/
#include "main.h"
#include "task_beacon.h"
#include "task_I2C.h"

// Pumpkin CubeSat Kit headers
#include "csk_io.h"
#include "csk_uart.h"
#include "csk_sd.h"
#include "thin_usr.h"

// Pumpkin Salvo headers
#include "salvo.h"
#include "task_Vand.h"

extern void timeStamp(char* aString);
extern char HeTrans255Str(char* inpt);
extern void VUC_getRunState(char* dat);
extern void	VUC_getStatus(char *dat);
extern void VUC_getTime(char* dat);

extern void getMissionClock(char* array);
extern char* RSSI_getTelem(int charOrAscii);
extern char* RSSI_getConfig(int charOrAscii);
extern char* VUC_getStoredTelem(int charOrAscii);
extern char * asciified3Array(unsigned int* a, int aLen);
extern unsigned int* i2c_getADC();
extern unsigned int i2c_getThisADCchannel(int channelID);

/******************************************************************************
****                                                                       ****
**                                                                           **
task_beacon()

The beacon broadcasts telemetry in frames. 
 00: The core frame data
 01: The core frame data + payload telemetry
 02: The core frame data + radio telemetry
 
 Each frame has a header with a start sequence and four 2-byte numbers
 
 SLUBCN[SC ID][VERSION][STATUS][FRAME ID]
 
 The remaining data is space-delimited; specific details are below
 
 The frame has a stop sequence  BEAEND
 
 ----
 The beacon period is set as the interval between frame 00 broadcasts.
 Separate counters are established for the other frames, specifically to be
 broadcast every Nth time. In other words, if one reduces the interval between
 frames, the number of times that the sub-frames will be heard each minute will
 increase.
**                                                                           **
****                                                                       ****
******************************************************************************/



/* The first is the 1-second intervals between any beacon broadcasts. The remaining
 * are the number of beacon broadcasts between sending that frame. See setFrameIntervals()
 * for more information.
 */
static unsigned int beaconFrameIntervals[BEACON_NUM_FRAMES+1] = {10, 6, 6};

/* Sets the interval between each frame and frame type. The first one is handled
    differently.
    nums[0] - the number of 1-second intervals between each frame (not just frame 0)
    nums[i] - the number of frames broadcast before sending frame i again.
 
    The frames are initialized to be pseudo-distributed (at least there's a good faith effort
    to spread them out). After that, it's every man for himself.

 When there's a conflict, the highest-frame goes first, and then the next-lowest goes on
 the next frame. Keep in mind that this is fairly brittle logic; if your frame intervals
 are all small numbers, unusual behaviors could result.

 */
void setBeaconFrameIntervals(unsigned int *nums) {
    int i;
    // We only have three frames
    for (i = 0; i < BEACON_NUM_FRAMES+1; i++) {
        beaconFrameIntervals[i] = nums[i];
    }
}

static char vucrun[2], vucstat[2], vutime[6];
static char final[400];
static unsigned int beaconWaitCounts[3];
static int frameID;

void task_beacon(void) {
//	unsigned char data;
	unsigned int i; //count, i;
//	F_FILE * BeaconFile;
//	char* tmpChar;
//	char tmpStr[2*CONFIG_LEN];
    // Initialize the interval counters; beacon on startup
	beaconWaitCounts[0]=beaconFrameIntervals[0];
    // Give a good-faith effort to offset the counters in various positions
    for (i=1; i <= BEACON_NUM_FRAMES; i++) {
        beaconWaitCounts[i] = beaconFrameIntervals[i]*(i-1)/BEACON_NUM_FRAMES;
    }
    
	while (1) {
		// Check the clock
		if (beaconWaitCounts[0] < beaconFrameIntervals[0]) {
			OS_Delay(100);
			beaconWaitCounts[0]++;
		} else {
			OSSignalBinSem(BINSEM_SEND_BEACON_P);
			beaconWaitCounts[0] = 0;
            // Now, figure out the frame
            frameID = BEACON_NUM_FRAMES;
            // Keep lowering the frameID until a wait counter is exceeded, or we reach 0
            while ((beaconWaitCounts[frameID] <beaconFrameIntervals[frameID]) 
					&& (frameID > 0)) {
                // If it doesn't match, increment the counter
                beaconWaitCounts[frameID]++;
                frameID--;
            }
            // Set the counter to zero. Yes, if frameID = 0, we're repeating ourselves.
            beaconWaitCounts[frameID] = 0;
 		}
		// Check the SEND_BEACON semaphore (sets low if high!)
		if (OSTryBinSem(BINSEM_SEND_BEACON_P)) {
			// Need to let task_commandMHX finish reading the buffer
            // Note that we will not wait forever; this prevents hangups
			OS_WaitBinSem(BINSEM_CLEAR_TO_SEND_P, 250);
            // Start with the header BEACON_START[SC ID][VERSION][STATUS][FRAME ID]
			int states = OSReadBinSem(BINSEM_DEPLOYED_P) + 2*OSReadBinSem(BINSEM_EJECTED_P)
                + 8*OSReadBinSem(BINSEM_BURNCIRCUIT_P) + 4*OSReadBinSem(BINSEM_CLEAR_TO_SEND_P);
            // Base frame: Mission_Clock [ADCs] VUC_Data Real_Time_Clock
            getMissionClockString(strTmp);

		 	sprintf(final, "%s%02X%02X%02X%02X %s ", BEACON_START, SC_ID,
				OS_VERSION, states, frameID, strTmp);
            
           // ======================= Switch on frame ID
			if (frameID == 0) {
//				sprintf(final, "%s %s ", final, 
//					asciified3Array(i2c_getADC(), NUM_ADC_CHANNELS));

				// VUC data
	            // [1-POWER][4-VUC STATUS][4-RUN STATUS][2-Reset count][4-Clock time]
				VUC_getRunState(vucrun);
				VUC_getStatus(vucstat);
				VUC_getTime(vutime);
				// VUC power state, status, run state
				sprintf(final,"%s%0d%02X%02X%02X%02X",
					final, (PORTF&BIT1)!=0, 
					vucstat[0], vucstat[1], vucrun[0], vucrun[1]);
				sprintf(final,"%s%02X%02X%02X%02X%02X%02X", final,
					vutime[0], vutime[1],vutime[2],vutime[3],vutime[4],vutime[5]);
            
				// Time Stamp (RTC) includes a leading space. Needs a trailer
				timeStamp(final);

   			} else if (frameID == 1) {
                // VUC Telemetry data
                sprintf(final, "%s %s", final, 
					//i2c_getThisADCchannel(ADC_BATV),
					VUC_getStoredTelem(2));
            } else if (frameID == 2) {
                // Helium config/telemetry data
                sprintf(final, "%s %s", final,
					//i2c_getThisADCchannel(ADC_BATV), 
					RSSI_getConfig(2));
				// These must be split into two sprintfs, or the array is
				// repeated!
                sprintf(final, "%s %s", final, RSSI_getTelem(2));
            }

            int n;
			// Finish it off
			sprintf(final,"%s%s\r\n%n",final, BEACON_END, &n);
			// Send
			HeTrans255Str(final);
			// Write to file and stdout
			char BeaconFilename[]=BEACON_FILE_NAME;
			F_FILE * BeaconFile=f_open(BeaconFilename,"a");
			f_write(final,1,strlen(final),BeaconFile);
			f_close(BeaconFile);
	
			csk_uart0_puts("\r\n");
			sprintf(final,"%s%d\r\n",final,n);
			csk_uart0_puts(final);			
		} /* End BEACON_SEND semaphore check */	
	}	/* End while(1) */
} /* task_beacon() */
