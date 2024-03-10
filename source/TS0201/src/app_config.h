#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#define VERSION 0x47	 // BCD format (0x34 -> '3.4')
#define EEP_SUP_VER 0x09 // EEP data minimum supported version

// DevID:
//#ifndef DEVICE_CGG1_ver
//#define DEVICE_CGG1_ver	0 // =2022 - CGG1-M version 2022, or = 0 - CGG1-M version 2020,2021
//#endif
//#define DEVICE_MHO_C401   1	// E-Ink display MHO-C401 2020
//#if DEVICE_CGG1_ver == 0
//#define DEVICE_CGG1 		2  // E-Ink display Old CGG1-M "Qingping Temp & RH Monitor"
//#else
//#define DEVICE_CGG1 		7  // E-Ink display New CGG1-M "Qingping Temp & RH Monitor"
//#endif
//#define DEVICE_CGDK2 		6  // LCD display "Qingping Temp & RH Monitor Lite"
//#define DEVICE_MHO_C401N   	 8	// E-Ink display MHO-C401 2022
//#define DEVICE_MJWSD05MMC	9  // LCD display MJWSD05MMC
//#define DEVICE_LYWSD03MMC 10	// LCD display LYWSD03MMC
//#define DEVICE_MHO_C122   11	// LCD display MHO_C122
// Ext.devID:
//#define BOARD_TB03F		16 // DIY TB-03F-Kit
#define DEVICE_TS0201   	17	// ZigBee TS0201, analog: IH-K009
//#define DEVICE_TNKS   	18	// DIY, PB-03F module, Water tank controller (not yet published at the moment)
//#define DEVICE_THB2		19	// PHY62x2 BLE
//#define DEVICE_BTH01		20	// PHY62x2 BLE
//#define DEVICE_TH05		21	// PHY62x2 BLE LCD
#define DEVICE_TH03Z   		22	// ZigBee TH03Z
//#define BOARD_THB1		23 // https://github.com/pvvx/THB2
//#define BOARD_TH05D		24 // TH05_V1.3 https://github.com/pvvx/THB2
//#define BOARD_TH05F		25 // TH05Y_V1.2 https://github.com/pvvx/THB2
//#define BOARD_THB3		26 // https://github.com/pvvx/THB2


#ifndef DEVICE_TYPE
#define DEVICE_TYPE			DEVICE_TH03Z // Use TS0201 or DEVICE_TH03Z only
#endif

// supported services by the device (bits)
#define SERVICE_OTA			0x00000001	// OTA all enable!
#define SERVICE_OTA_EXT		0x00000002	// Compatible BigOTA
#define SERVICE_PINCODE 	0x00000004	// support pin-code
#define SERVICE_BINDKEY 	0x00000008	// support encryption beacon (bindkey)
#define SERVICE_HISTORY 	0x00000010	// flash logger enable
#define SERVICE_SCREEN		0x00000020	// screen enable
#define SERVICE_LE_LR		0x00000040	// support extension advertise + LE Long Range
#define SERVICE_THS			0x00000080	// all enable
#define SERVICE_RDS			0x00000100	// wake up when the reed switch is triggered + pulse counter
#define SERVICE_KEY			0x00000200	//
#define SERVICE_OUTS		0x00000400	//
#define SERVICE_INS			0x00000800	//
#define SERVICE_TIME_ADJUST 0x00001000	// time correction enabled
#define SERVICE_HARD_CLOCK	0x00002000	// RTC enabled
#define SERVICE_TH_TRG		0x00004000	// use trigger out

#define DEV_SERVICES ( SERVICE_OTA\
		| SERVICE_OTA_EXT \
		| SERVICE_PINCODE \
		| SERVICE_BINDKEY \
		| SERVICE_HISTORY \
		| SERVICE_LE_LR \
		| SERVICE_THS \
		| SERVICE_RDS \
		| SERVICE_KEY \
		| SERVICE_TIME_ADJUST \
		| SERVICE_TH_TRG \
)

#define BLE_SECURITY_ENABLE 1 // = 1 support pin-code
#define BLE_EXT_ADV 		1 // = 1 support extension advertise (Test Only!)

#define USE_CLOCK 			1 // = 1 display clock, = 0 smile blinking
#define USE_TIME_ADJUST		1 // = 1 time correction enabled
#define USE_FLASH_MEMO		1 // = 1 flash logger enable

#define USE_SECURITY_BEACON 1 // = 1 support encryption beacon (bindkey)
#define USE_HA_BLE_BEACON	1 // = 1 BTHome v1 https://bthome.io/
#define USE_MIHOME_BEACON	1 // = 1 Compatible with MiHome beacon
#define USE_BTHOME_BEACON	1 // = 1 BTHome v2 https://bthome.io/

#define USE_KEYS_WAKEAP		1 // = 1 опрос кнопки с прерыванием

#define USE_OUT_AVERAGE_BATTERY	0 // = 1 Transmit averaged values

#define USE_EXT_OTA			1 // = 1 Compatible BigOTA

#define USE_DEVICE_INFO_CHR_UUID 	1 // = 1 enable Device Information Characteristics
#define USE_FLASH_SERIAL_UID		1 // =1 Set my_SerialStr "$SOC_ID_Rev-$FLASH_JEDEC-$FLASH_UID"

#define UART_PRINT_DEBUG_ENABLE		0 // =1 use u_printf() (PA7/SWS), source: SDK/components/application/print/u_printf.c

#define ZIGBEE_TUYA_OTA 	1

#if DEVICE_TYPE == DEVICE_TS0201

// GPIO_PB1 - TX
// GPIO_PB4 - LED
// GPIO_PB7 - RX
// GPIO_PC0 - KEY
// GPIO_PC2 - SDA
// GPIO_PC3 - SCL
// GPIO_PD7 - Alert

#define SHL_ADC_VBAT	1  // "B0P" in adc.h
#define GPIO_VBAT	GPIO_PB0 // missing pin on case TLSR8251F512ET24
#define PB0_INPUT_ENABLE	1
#define PB0_DATA_OUT		1
#define PB0_OUTPUT_ENABLE	1
#define PB0_FUNC			AS_GPIO

#define I2C_SCL 	GPIO_PC3
#define I2C_SDA 	GPIO_PC2
#define I2C_GROUP 	I2C_GPIO_GROUP_C2C3
#define PULL_WAKEUP_SRC_PC2	PM_PIN_PULLUP_10K
#define PULL_WAKEUP_SRC_PC3	PM_PIN_PULLUP_10K

#define GPIO_KEY			GPIO_PC0
#define PC0_INPUT_ENABLE	1
#define PC0_DATA_OUT		0
#define PC0_OUTPUT_ENABLE	0
#define PC0_FUNC			AS_GPIO
#define PULL_WAKEUP_SRC_PC0	PM_PIN_PULLUP_10K

#define GPIO_LED			GPIO_PB4
#define PB4_INPUT_ENABLE	1
#define PB4_DATA_OUT		1
#define PB4_OUTPUT_ENABLE	0
#define PB4_FUNC			AS_GPIO
#define PULL_WAKEUP_SRC_PB4	PM_PIN_PULLDOWN_100K

#elif DEVICE_TYPE == DEVICE_TH03Z

// GPIO_PB1 - TX
// GPIO_PB4 - LED
// GPIO_PB7 - RX
// GPIO_PC0 - KEY
// GPIO_PC2 - SDA
// GPIO_PC3 - SCL
// GPIO_PD7 - Alert

#define SHL_ADC_VBAT	1  // "B0P" in adc.h
#define GPIO_VBAT	GPIO_PB0 // missing pin on case TLSR8251F512ET24
#define PB0_INPUT_ENABLE	1
#define PB0_DATA_OUT		1
#define PB0_OUTPUT_ENABLE	1
#define PB0_FUNC			AS_GPIO

#define I2C_SCL 	GPIO_PC3
#define I2C_SDA 	GPIO_PC2
#define I2C_GROUP 	I2C_GPIO_GROUP_C2C3
#define PULL_WAKEUP_SRC_PC2	PM_PIN_PULLUP_10K
#define PULL_WAKEUP_SRC_PC3	PM_PIN_PULLUP_10K

#define GPIO_KEY			GPIO_PC0
#define PC0_INPUT_ENABLE	1
#define PC0_DATA_OUT		0
#define PC0_OUTPUT_ENABLE	0
#define PC0_FUNC			AS_GPIO
#define PULL_WAKEUP_SRC_PC0	PM_PIN_PULLUP_10K

#define GPIO_LED			GPIO_PB4
#define PB4_INPUT_ENABLE	1
#define PB4_DATA_OUT		1
#define PB4_OUTPUT_ENABLE	0
#define PB4_FUNC			AS_GPIO
#define PULL_WAKEUP_SRC_PB4	PM_PIN_PULLDOWN_100K

#else // DEVICE_TYPE
#error ("DEVICE_TYPE = ?")
#endif // DEVICE_TYPE == ?

#if UART_PRINT_DEBUG_ENABLE
#define PRINT_BAUD_RATE 1500000 // real ~1000000
#define DEBUG_INFO_TX_PIN	GPIO_PA7 // SWS
#define PA7_DATA_OUT		1
#define PA7_OUTPUT_ENABLE	1
#define PULL_WAKEUP_SRC_PA7 PM_PIN_PULLUP_1M
#define PA7_FUNC		AS_GPIO
#endif // UART_PRINT_DEBUG_ENABLE

#define MODULE_WATCHDOG_ENABLE		0 //
#define WATCHDOG_INIT_TIMEOUT		15000  //ms (min 5000 ms if pincode)

#define CLOCK_SYS_CLOCK_HZ  	24000000 // 16000000, 24000000, 32000000, 48000000
enum{
	CLOCK_SYS_CLOCK_1S = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};

#define pm_wait_ms(t) cpu_stall_wakeup_by_timer0(t*CLOCK_SYS_CLOCK_1MS);
#define pm_wait_us(t) cpu_stall_wakeup_by_timer0(t*CLOCK_SYS_CLOCK_1US);

#define RAM _attribute_data_retention_ // short version, this is needed to keep the values in ram after sleep

/* Flash map:
  0x00000 Firmware bin or OTA1 bin storage Area (OTA1_FADDR)
  0x20000 Firmware bin or OTA2 bin storage Area (OTA2_FADDR)
  0x40000 User Data Area, if (Logger, saving measurements) (FLASH_ADDR_START_MEMO)
  0x74000 Pair & Security info (CFG_ADR_BIND)
  0x76000 MAC address (CFG_ADR_MAC)
  0x77000 Customize freq_offset adjust cap value (CUST_CAP_INFO_ADDR)
  0x78000 free: Used Master BLE (CFG_ADR_PEER)
  0x7С000 EEP Data Area (FMEMORY_SCFG_BASE_ADDR)
  0x80000 End Flash 512KB (FLASH_SIZE)
  if (Flash 1MB)
  	  0x80000 Logger, saving measurements (FLASH_ADDR_START_MEMO)
  	  0x100000 End Flash 1MB (FLASH_SIZE)
 */
#define OTA1_FADDR 0x00000
#define OTA2_FADDR 0x20000
/* flash sector address with binding information */
#define	CFG_ADR_BIND	0x74000 //no master, slave device (blt_config.h)

#define BLE_HOST_SMP_ENABLE BLE_SECURITY_ENABLE

//#define CHG_CONN_PARAM	// test
#define DEV_NAME "pvvx_ble" // not used (see SDK "blt_config.h")

#include "vendor/common/default_config.h"

#if defined(__cplusplus)
}
#endif
