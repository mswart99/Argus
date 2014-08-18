/******************************************************************************
(C) Copyright Pumpkin, Inc. All Rights Reserved.

This file may be distributed under the terms of the License
Agreement provided with this software.

THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE.

$Source: C:\\RCS\\D\\Pumpkin\\CubeSatKit\\Example\\all\\all\\CubeSatKit_Dev_Board\\Test\\Test1\\task_5sec.h,v $
$Author: aek $
$Revision: 3.0 $
$Date: 2009-10-11 22:38:16-07 $

******************************************************************************/
#ifndef __task_beacon_h
#define __task_beacon_h


// Function declarations
extern void task_beacon(void);

// Symbols, etc.
#define STR_TASK_BEACON "task_beacon: " STR_1TAB

// Beacon parameters
#define BEACON_START 	"SLUBCN"
#define BEACON_END 		"BEAEND"
#define BEACON_FILE_NAME "BEACON"
#define BEACON_NUM_FRAMES 		2

#endif /* __task_beacon_h */

