/*
 * app.h
 *
 *  Created on: 19.12.2020
 *      Author: pvvx
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "drivers/8258/gpio_8258.h"

enum {
	HW_VER_LYWSD03MMC_B14 = 0,
	HW_VER_MHO_C401,		//1
	HW_VER_CGG1,			//2
	HW_VER_LYWSD03MMC_B19,	//3
	HW_VER_LYWSD03MMC_B16,	//4
	HW_VER_LYWSD03MMC_B17,	//5
	HW_VER_CGDK2,			//6
	HW_VER_CGG1_2022,		//7
	HW_VER_MHO_C401_2022,	//8
	HW_VER_MJWSD05MMC,		//9
	HW_VER_LYWSD03MMC_B15,	//10
	HW_VER_MHO_C122,		//11
	HW_VER_UNKNOWN = 15
} HW_VERSION_ID;

// Adv. types
enum {
	ADV_TYPE_BTHOME = 0,
	ADV_TYPE_PVVX, // (default)
	ADV_TYPE_MI,
	ADV_TYPE_HA_BLE
} ADV_TYPE_ENUM;

#define ADV_TYPE_DEFAULT	ADV_TYPE_PVVX

#define MASK_FLG2_REBOOT	0x60
#define MASK_FLG2_SCR_OFF	0x80

typedef struct __attribute__((packed)) _cfg_t {
	struct __attribute__((packed)) {
		uint8_t advertising_type	: 2; // 0 - atc1441, 1 - Custom (pvvx), 2 - Mi, 3 - HA_BLE
		uint8_t reserved1			: 2; // comfort_smiley, show_time_smile
		uint8_t temp_F_or_C			: 1;
		uint8_t reserved2			: 1; // show_batt_enabled
		uint8_t tx_measures			: 1; // Send measurements in connected mode
		uint8_t reserved3			: 1; // lp_measures
	} flg;
	struct __attribute__((packed)) {
		uint8_t reserved1			: 3; // screen_type
		uint8_t adv_crypto			: 1; // advertising uses crypto beacon
		uint8_t adv_flags  			: 1; // advertising add flags
		uint8_t bt5phy  			: 1; // support BT5.0 All PHY
		uint8_t longrange  			: 1; // advertising in LongRange mode (сбрасывается после отключения питания)
		uint8_t reserved			: 1; // screen_off
	} flg2;
	int8_t temp_offset; // Set temp offset, -12,5 - +12,5 °C (-125..125)
	int8_t humi_offset; // Set humi offset, -12,5 - +12,5 % (-125..125)
	uint8_t advertising_interval; // multiply by 62.5 for value in ms (1..160,  62.5 ms .. 10 sec)
	uint8_t measure_interval; // measure_interval = advertising_interval * x (2..25)
	uint8_t rf_tx_power; // RF_POWER_N25p18dBm .. RF_POWER_P3p01dBm (130..191)
	uint8_t connect_latency; // +1 x0.02 sec ( = connection interval), Tmin = 1*20 = 20 ms, Tmax = 256 * 20 = 5120 ms
	uint8_t ext_hw_id;	// old: min_step_time_update_lcd, new -> if (hwver == 15) use ext_hw_ver
	struct __attribute__((packed)) {
		uint8_t hwver		: 4; // 0..14, =15 - use ext_hw_ver
		uint8_t reserved	: 4; // reserved
	} hw_cfg; // read only
	uint8_t averaging_measurements; // * measure_interval, 0 - off, 1..255 * measure_interval
}cfg_t;
extern cfg_t cfg;
extern const cfg_t def_cfg;

extern uint32_t utc_time_sec;	// clock in sec (= 0 1970-01-01 00:00:00)
#if	USE_TIME_ADJUST
extern uint32_t utc_time_tick_step; // adjust time clock (in 1/16 us for 1 sec)
#endif

#if BLE_SECURITY_ENABLE
extern uint32_t pincode; // pincode (if = 0 - not used)
#endif

typedef struct _measured_data_t {
#if USE_OUT_AVERAGE_BATTERY
	uint16_t 	average_battery_mv; // mV
#else
	uint16_t	battery_mv; // mV
#endif
	int16_t		temp; // x 0.01 C
	int16_t		humi; // x 0.01 %
	uint16_t 	count;
#if USE_OUT_AVERAGE_BATTERY
	uint16_t	battery_mv; // mV
#else
	uint16_t 	average_battery_mv; // mV
#endif
	int16_t 	temp_x01; 		// x 0.1 C
	int16_t		humi_x01; 		// x 0.1 %
	uint8_t 	humi_x1; 		// x 1 %
	uint8_t 	battery_level;	// 0..100% (average_battery_mv)
} measured_data_t;
#define  MEASURED_MSG_SIZE  8

extern measured_data_t measured_data;

extern volatile uint8_t tx_measures; // measurement transfer counter, flag
extern volatile uint8_t start_measure; // start measurements
extern volatile uint8_t wrk_measure; // measurements in process
extern uint8_t end_measure; // measurements completed

extern uint32_t tim_measure; // measurement timer

typedef struct _comfort_t {
	int16_t  t[2];
	uint16_t h[2];
}scomfort_t, * pcomfort_t;
extern scomfort_t cmf;

#if USE_SECURITY_BEACON
extern uint8_t bindkey[16];
void bindkey_init(void);
#endif

#if BLE_SECURITY_ENABLE
extern uint32_t pincode;
#endif

extern uint32_t adv_interval; // adv interval in 0.625 ms // = cfg.advertising_interval * 100
extern uint32_t connection_timeout; // connection timeout in 10 ms, Tdefault = connection_latency_ms * 4 = 2000 * 4 = 8000 ms
extern uint32_t measurement_step_time; // = adv_interval * measure_interval

void set_default_cfg(void);
void ev_adv_timeout(u8 e, u8 *p, int n); // DURATION_TIMEOUT Event Callback
void test_config(void); // Test config values

uint8_t * str_bin2hex(uint8_t *d, uint8_t *s, int len);

//---- blt_common.c
void blc_newMacAddress(int flash_addr, u8 *mac_pub, u8 *mac_rand);
void SwapMacAddress(u8 *mac_out, u8 *mac_in);
void flash_erase_mac_sector(u32 faddr);

#endif /* MAIN_H_ */
