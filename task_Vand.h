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
#ifndef __task_Vand_h
#define __task_Vand_h


// Function declarations
extern void task_Vand(void);

// Symbols, etc.
#define STR_TASK_VAND "task_Vand: " STR_1TAB
#define VUC_I2C_ADDRESS			0x20
#define VAND_I2C_ADDRESS		0x38
#define VUC_MSG_VER_HI			0x00
#define VUC_MSG_VER_LO			0x65
#define VUC_SW_VER_HI			0x00
#define VUC_SW_VER_LO			0x6E
#define VUC_OFF						0x00
#define VUC_STANDBY					0x01
#define VUC_DIAGNOSTIC				0x02
#define VUC_ACTIVE					0x03
#define VUC_DISABLED				0x04
#define VUC_HALT					0x05
#define VUC_BYTE1_COMMAND			0x00
#define VUC_NOP						0x00
#define VUC_ECHO					0x01
#define VUC_RESEND					0x02
#define VUC_GET_UID					0x03
#define VUC_GET_STATUS				0x04
#define VUC_GET_TELEM				0x10
#define VUC_SET_RUNSTATE			0x80
#define VUC_GET_RUNSTATE			0x81
#define VUC_BYTE1_TIME				0x01
#define VUC_SET_TIME				0x00
#define VUC_GET_TIME				0x01
#define VUC_BYTE1_STORAGE			0x02
#define VUC_GET_DATA				0x80
#define VUC_BYTE1_LOGGER			0x03
#define VUC_SET_LOGVERBOSITY		0x00
#define VUC_GET_LOGVERBOSITY		0x01
#define VUC_TELEM_LEN	100

#endif /* __task_Vand_h */

