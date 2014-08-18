#ifndef __helium_h
#define __helium_h

// Radio packet lengths
#define HE_TELEM_LEN 			  14
#define HE_CONFIG_LEN			  34

// Symbols, etc.
#define BUFFER_LEN				  400
#define SYNC1				 	  'H'
#define SYNC2					  'e'
#define SYNC3					  0x20
#define HE_TX					  0x40
#define NO_OP_COMMAND			  0x01
#define RESET_SYSTEM			  0x02
#define TRANSMIT_DATA			  0x03
#define RECEIVE_DATA			  0x04
#define GET_TRANSCEIVER_CONFIG    0x05
#define SET_TRANSCEIVER_CONFIG    0x06
#define TELEMETRY_QUERY			  0x07
#define WRITE_FLASH				  0x08
#define RF_CONFIG				  0x09
#define BEACON_DATA				  0x10
#define BEACON_CONFIG			  0x11
#define READ_FIRMWARE_REVISION	  0x12
#define WRITE_OVER_AIR_KEY		  0x13
#define FIRMWARE_UPDATE			  0x14
#define FIRMWARE_PACKET			  0x15
#define FAST_PA_SET				  0x20
#define CALL0	'A'
#define CALL1	'R'
#define CALL2	'G'
#define CALL3	'U'
#define CALL4	'S'
#define CALL5	'1'
#define GROUND0	'S'
#define GROUND1	'L'
#define GROUND2	'U'
#define GROUND3	'G'
#define GROUND4	'N'
#define GROUND5	'D'
#define HE_BAUD_RATE_9600 		0x00
#define HE_BAUD_RATE_19200 		0x01
#define HE_BAUD_RATE_38400 		0x02
#define HE_BAUD_RATE_76800 		0x03
#define HE_BAUD_RATE_115200 	0x04

#endif /* __helium_h */






