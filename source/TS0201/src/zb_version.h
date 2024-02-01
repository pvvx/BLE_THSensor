/*********************************************************************************************
 * During OTA upgrade, the upgraded device will check the rules of the following three fields.
 * Refer to ZCL OTA specification for details.
 */

#define APP_RELEASE							0x99 // app release 9.9
#define APP_BUILD							0x99 // app build 9.9
#define STACK_RELEASE						0x30 // stack release 3.0
#define STACK_BUILD							0x01 // stack build 01

// DevID:
#ifndef DEVICE_CGG1_ver
#define DEVICE_CGG1_ver		 0 // =2022 - CGG1-M version 2022, or = 0 - CGG1-M version 2020,2021
#endif
#define DEVICE_MHO_C401   	 1	// E-Ink display MHO-C401 2020
#if DEVICE_CGG1_ver == 0     
#define DEVICE_CGG1 		 2  // E-Ink display Old CGG1-M "Qingping Temp & RH Monitor"
#else
#define DEVICE_CGG1 		 7  // E-Ink display New CGG1-M "Qingping Temp & RH Monitor"
#endif
#define DEVICE_CGDK2 		 6  // LCD display "Qingping Temp & RH Monitor Lite"
#define DEVICE_MHO_C401N   	 8	// E-Ink display MHO-C401 2022
#define DEVICE_MJWSD05MMC	 9  // LCD display MJWSD05MMC
#define DEVICE_LYWSD03MMC   10	// LCD display LYWSD03MMC
#define DEVICE_MHO_C122   	11	// LCD display MHO_C122
// Ext.devID:
#define DEVICE_TNK01   		16	// DIY, PB-03F module
#define DEVICE_TS0201   	17	// ZigBee TS0201, analog: IH-K009

#define DEVICE_ORIG_TYUA_TS0201  0xA3D3 // original Tuya TS0201  _TZ3000_qsefbina _TZ3000_xr3htd96


#ifndef DEVICE_TYPE
#define DEVICE_TYPE DEVICE_TS0201
#endif

/* Chip IDs */
#define CHIP_TYPE_8267							0x00
#define CHIP_TYPE_8269							0x01
#define CHIP_TYPE_8258							0x02
#define CHIP_TYPE_8258_1M						0x03
#define CHIP_TYPE_8278							0x04
#define CHIP_TYPE_B91							0x05


#define MANUFACTURER_CODE_TELINK           	0x1141 // Telink ID

#define CHIP_TYPE CHIP_TYPE_8258

#define	IMAGE_TYPE			((CHIP_TYPE << 8) | DEVICE_TYPE)
#define	FILE_VERSION		((APP_RELEASE << 24) | (APP_BUILD << 16) | (STACK_RELEASE << 8) | STACK_BUILD)
