#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_common.h"
#include "cmd_parser.h"
#include "flash_eep.h"
#include "battery.h"
#include "ble.h"
#include "sensor.h"
#include "app.h"
#include "i2c.h"
#include "keys.h"
#if USE_FLASH_MEMO
#include "logger.h"
#endif
#if USE_MIHOME_BEACON
#include "mi_beacon.h"
#endif
#if USE_HA_BLE_BEACON
#include "ha_ble_beacon.h"
#endif
#if USE_BTHOME_BEACON
#include "bthome_beacon.h"
#endif
#if	USE_EXT_OTA
#include "ext_ota.h"
#endif


void app_enter_ota_mode(void);

RAM measured_data_t measured_data;

RAM volatile uint8_t tx_measures; // measurement transfer counter, flag
RAM volatile uint8_t start_measure; // start measurements
RAM volatile uint8_t wrk_measure; // measurements in process
RAM uint8_t end_measure; // measurements completed
RAM uint32_t tim_measure; // measurement timer

RAM uint32_t adv_interval; // adv interval in 0.625 ms // = cfg.advertising_interval * 100
RAM uint32_t connection_timeout; // connection timeout in 10 ms, Tdefault = connection_latency_ms * 4 = 2000 * 4 = 8000 ms
RAM uint32_t measurement_step_time; // = adv_interval * measure_interval

RAM uint32_t utc_time_sec;	// clock in sec (= 0 1970-01-01 00:00:00)
RAM uint32_t utc_time_sec_tick; // clock counter in 1/16 us
#if USE_TIME_ADJUST
RAM uint32_t utc_time_tick_step = CLOCK_16M_SYS_TIMER_CLK_1S; // adjust time clock (in 1/16 us for 1 sec)
#else
#define utc_time_tick_step CLOCK_16M_SYS_TIMER_CLK_1S
#endif

#if BLE_SECURITY_ENABLE
RAM uint32_t pincode;
#endif

#if USE_SECURITY_BEACON
RAM uint8_t bindkey[16];
#endif

// Settings
const cfg_t def_cfg = {
		.flg2.adv_flags = true,
		.flg.advertising_type = ADV_TYPE_DEFAULT,
		.rf_tx_power = RF_POWER_P0p04dBm, // RF_POWER_P3p01dBm,
		.connect_latency = DEF_CONNECT_LATENCY, // (49+1)*1.25*16 = 1000 ms
		.advertising_interval = 40, // multiply by 62.5 ms = 2.5 sec
		.measure_interval = 4, // * advertising_interval = 10 sec
		.flg.temp_F_or_C = false,
		.hw_cfg.hwver = 0xf,
		.ext_hw_id = (USE_EXT_OTA << 7) | (DEVICE_TYPE - 16),
#if USE_FLASH_MEMO
		.averaging_measurements = 180, // * measure_interval = 10 * 180 = 1800 sec = 30 minutes
#endif
		};
RAM cfg_t cfg;

// go deep-sleep 
void go_sleep(uint32_t tik) {
	cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_TIMER,
				clock_time() + tik); 
	while(1);
}

__attribute__((optimize("-Os")))
void test_config(void) {
	if (cfg.flg2.longrange)
		cfg.flg2.bt5phy = 1;
	if (cfg.rf_tx_power & BIT(7)) {
		if (cfg.rf_tx_power < RF_POWER_N25p18dBm)
			cfg.rf_tx_power = RF_POWER_N25p18dBm;
		else if (cfg.rf_tx_power > RF_POWER_P3p01dBm)
			cfg.rf_tx_power = RF_POWER_P3p01dBm;
	} else {
		if (cfg.rf_tx_power < RF_POWER_P3p23dBm)
			cfg.rf_tx_power = RF_POWER_P3p23dBm;
		else if (cfg.rf_tx_power > RF_POWER_P10p46dBm)
			cfg.rf_tx_power = RF_POWER_P10p46dBm;
	}
	if (cfg.measure_interval < 2)
		cfg.measure_interval = 2; // T = cfg.measure_interval * advertising_interval_ms (ms),  Tmin = 1 * 1*62.5 = 62.5 ms / 1 * 160 * 62.5 = 10000 ms
	else if (cfg.measure_interval > 25) // max = (0x100000000-1.5*10000000*16)/(10000000*16) = 25.3435456
		cfg.measure_interval = 25; // T = cfg.measure_interval * advertising_interval_ms (ms),  Tmax = 25 * 160*62.5 = 250000 ms = 250 sec
	if (cfg.flg.tx_measures)
		tx_measures = 0xff; // always notify
	if (cfg.advertising_interval == 0) // 0 ?
		cfg.advertising_interval = 1; // 1*62.5 = 62.5 ms
	else if (cfg.advertising_interval > 160) // max 160 : 160*62.5 = 10000 ms
		cfg.advertising_interval = 160; // 160*62.5 = 10000 ms
	adv_interval = cfg.advertising_interval * 100; // Tadv_interval = adv_interval * 62.5 ms
	measurement_step_time = adv_interval * cfg.measure_interval * (625
			* sys_tick_per_us) - 250; // measurement_step_time = adv_interval * 62.5 * measure_interval, max 250 sec

	if(cfg.connect_latency > DEF_CONNECT_LATENCY && measured_data.average_battery_mv < LOW_VBAT_MV)
		cfg.connect_latency = DEF_CONNECT_LATENCY;
	/* interval = 16;
	 * connection_interval_ms = (interval * 125) / 100;
	 * connection_latency_ms = (cfg.connect_latency + 1) * connection_interval_ms = (16*125/100)*(99+1) = 2000;
	 * connection_timeout_ms = connection_latency_ms * 4 = 2000 * 4 = 8000;
	 */
	connection_timeout = ((cfg.connect_latency + 1) * (4 * DEF_CON_INERVAL * 125)) / 1000; // = 800, default = 8 sec
	if (connection_timeout > 32 * 100)
		connection_timeout = 32 * 100; //x10 ms, max 32 sec?
	else if (connection_timeout < 100)
		connection_timeout = 100;	//x10 ms,  1 sec

	if (!cfg.connect_latency) {
		my_periConnParameters.intervalMin =	(cfg.advertising_interval * 625	/ 30) - 1; // Tmin = 20*1.25 = 25 ms, Tmax = 3333*1.25 = 4166.25 ms
		my_periConnParameters.intervalMax = my_periConnParameters.intervalMin + 5;
		my_periConnParameters.latency = 0;
	} else {
		my_periConnParameters.intervalMin = DEF_CON_INERVAL; // 16*1.25 = 20 ms
		my_periConnParameters.intervalMax = DEF_CON_INERVAL; // 16*1.25 = 20 ms
		my_periConnParameters.latency = cfg.connect_latency;
	}
	my_periConnParameters.timeout = connection_timeout;
	cfg.hw_cfg.hwver = 0x0f;
	cfg.ext_hw_id = (USE_EXT_OTA << 7) | (DEVICE_TYPE - 16);
	my_RxTx_Data[0] = CMD_ID_CFG;
	my_RxTx_Data[1] = VERSION;
	memcpy(&my_RxTx_Data[2], &cfg, sizeof(cfg));
}

void low_vbat(void) {
	go_sleep(120 * CLOCK_16M_SYS_TIMER_CLK_1S); // go deep-sleep 2 minutes
}

_attribute_ram_code_
void WakeupLowPowerCb(int par) {
	(void) par;
	if (wrk_measure) {
		if (read_sensor()) {
			measured_data.count++;
			measured_data.temp_x01 = (measured_data.temp + 5)/ 10;
			measured_data.humi_x01 = (measured_data.humi + 5)/ 10;
			//measured_data.humi_x1 = (measured_data.humi + 50)/ 100;
#if USE_FLASH_MEMO
			if (cfg.averaging_measurements)
				write_memo();
#endif
		}
		end_measure = 0xff;
		wrk_measure = 0;
	}
}

_attribute_ram_code_
static void suspend_exit_cb(u8 e, u8 *p, int n) {
	(void) e; (void) p; (void) n;
	rf_set_power_level_index(cfg.rf_tx_power);
}

#if USE_KEYS_WAKEAP
_attribute_ram_code_
static void suspend_enter_cb(u8 e, u8 *p, int n) {
	(void) e; (void) p; (void) n;
	cpu_set_gpio_wakeup(GPIO_KEY, BM_IS_SET(reg_gpio_in(GPIO_KEY), GPIO_KEY & 0xff)? Level_Low : Level_High, 1);  // pad wakeup deepsleep enable
	bls_pm_setWakeupSource(PM_WAKEUP_PAD | PM_WAKEUP_TIMER);  // gpio pad wakeup suspend/deepsleep
}
#endif

//--- check battery
#define BAT_AVERAGE_SHL		4 // 16*16 = 256 ( 256*10/60 = 42.7 min)
#define BAT_AVERAGE_COUNT	(1 << BAT_AVERAGE_SHL) // 8
RAM struct {
	uint32_t buf2[BAT_AVERAGE_COUNT];
	uint16_t buf1[BAT_AVERAGE_COUNT];
	uint8_t index1;
	uint8_t index2;
} bat_average;

_attribute_ram_code_
__attribute__((optimize("-Os")))
void check_battery(void) {
	uint32_t i;
	uint32_t summ = 0;
	measured_data.battery_mv = get_battery_mv();
	if (measured_data.battery_mv < END_VBAT_MV) // It is not recommended to write Flash below 2V
		low_vbat();
	bat_average.index1++;
	bat_average.index1 &= BAT_AVERAGE_COUNT - 1;
	if(bat_average.index1 == 0) {
		bat_average.index2++;
		bat_average.index2 &= BAT_AVERAGE_COUNT - 1;
	}
	bat_average.buf1[bat_average.index1] = measured_data.battery_mv;
	for(i = 0; i < BAT_AVERAGE_COUNT; i++)
		summ += bat_average.buf1[i];
	bat_average.buf2[bat_average.index2] = summ;
	summ = 0;
	for(i = 0; i < BAT_AVERAGE_COUNT; i++)
		summ += bat_average.buf2[i];
	measured_data.average_battery_mv = summ >> (2*BAT_AVERAGE_SHL);
	measured_data.battery_level = get_battery_level(measured_data.average_battery_mv);
}

__attribute__((optimize("-Os")))
static void start_tst_battery(void) {
	int i;
	uint16_t avr_mv = get_battery_mv();
	measured_data.battery_mv = avr_mv;
	if (avr_mv < MIN_VBAT_MV) { // 2.2V
		// Set sleep power < 1 uA
		// send_i2c_word(0x70 << 1, 0x98b0); // SHTC3 go SLEEP: Sleep command of the sensor
		go_sleep(120 * CLOCK_16M_SYS_TIMER_CLK_1S); // go deep-sleep 2 minutes
	}
	measured_data.average_battery_mv = avr_mv;
	for(i = 0; i < BAT_AVERAGE_COUNT; i++)
		bat_average.buf1[i] = avr_mv;
	avr_mv <<= BAT_AVERAGE_SHL;
	for(i = 0; i < BAT_AVERAGE_COUNT; i++)
		bat_average.buf2[i] = avr_mv;
}

#if USE_SECURITY_BEACON
void bindkey_init(void) {
	if (flash_read_cfg(&bindkey, EEP_ID_KEY, sizeof(bindkey))
				!= sizeof(bindkey)) {
		generateRandomNum(sizeof(bindkey), (unsigned char *) &bindkey);
		flash_write_cfg(&bindkey, EEP_ID_KEY, sizeof(bindkey));
	}
#if	USE_MIHOME_BEACON
	mi_beacon_init();
#endif // USE_MIHOME_BEACON
#if USE_HA_BLE_BEACON
	ha_ble_beacon_init();
#endif
#if USE_BTHOME_BEACON
	bthome_beacon_init();
#endif
}
#endif // USE_SECURITY_BEACON

void set_default_cfg(void) {
	memcpy(&cfg, &def_cfg, sizeof(cfg));
	test_config();
	flash_write_cfg(&cfg, EEP_ID_CFG, sizeof(cfg));
	go_sleep(2*CLOCK_16M_SYS_TIMER_CLK_1S); // go deep-sleep 2 sec
}

//=========================================================
//-------------------- user_init_normal -------------------
void user_init_normal(void) {//this will get executed one time after power up
	bool next_start = false;
	unsigned int old_ver;
	adc_power_on_sar_adc(0); // - 0.4 mA
	lpc_power_down();
	blc_ll_initBasicMCU(); //must
	start_tst_battery();
	flash_unlock();
	random_generator_init(); //must
#if !ZIGBEE_TUYA_OTA // USE_EXT_OTA
	big_to_low_ota(); // Correct FW OTA address? Reformat Big OTA to Low OTA
#endif
	// Read config
	if(flash_read_cfg(&old_ver, EEP_ID_VER, sizeof(old_ver)) != sizeof(old_ver))
		old_ver = 0;
	next_start = flash_supported_eep_ver(EEP_SUP_VER, VERSION);
	if (next_start) {
		if (flash_read_cfg(&cfg, EEP_ID_CFG, sizeof(cfg)) != sizeof(cfg))
			memcpy(&cfg, &def_cfg, sizeof(cfg));
#if USE_TIME_ADJUST
		if (flash_read_cfg(&utc_time_tick_step, EEP_ID_TIM,
				sizeof(utc_time_tick_step)) != sizeof(utc_time_tick_step))
			utc_time_tick_step = CLOCK_16M_SYS_TIMER_CLK_1S;
#endif
#if BLE_SECURITY_ENABLE
		if (flash_read_cfg(&pincode, EEP_ID_PCD, sizeof(pincode))
				!= sizeof(pincode))
			pincode = 0;
#endif
#if (DEV_SERVICES & SERVICE_THS)
		if (flash_read_cfg(&thsensor.coef, EEP_ID_CFS, sizeof(thsensor.coef))
				!= sizeof(thsensor.coef))
			memset(&thsensor.coef, 0, sizeof(thsensor.coef));
#endif
	} else {
#if BLE_SECURITY_ENABLE
		pincode = 0;
#endif
		memcpy(&cfg, &def_cfg, sizeof(cfg));
	}
	init_i2c();
	reg_i2c_speed = (uint8_t)(CLOCK_SYS_CLOCK_HZ/(4*100000)); // 100 kHz
	test_config();
#if BLE_EXT_ADV
	if(analog_read(DEEP_ANA_REG0) != 0x55) {
		cfg.flg2.longrange = 0;
		flash_write_cfg(&cfg, EEP_ID_CFG, sizeof(cfg));
		analog_write(DEEP_ANA_REG0, 0x55);
	}
#endif // BLE_EXT_ADV
	init_ble();
	bls_app_registerEventCallback(BLT_EV_FLAG_SUSPEND_EXIT, &suspend_exit_cb);
#if USE_KEYS_WAKEAP
	bls_app_registerEventCallback(BLT_EV_FLAG_SUSPEND_ENTER, &suspend_enter_cb);
#endif
	start_tst_battery();
	init_sensor();
#if USE_FLASH_MEMO
	memo_init();
#endif
	wrk_measure = 1;
#if USE_SECURITY_BEACON
	bindkey_init();
#endif
	check_battery();
	WakeupLowPowerCb(0);
	if (!next_start) { // first start?
		flash_write_cfg(&cfg, EEP_ID_CFG, sizeof(cfg));
	}
	test_config();
	start_measure = 1;
}

//=========================================================
//------------------ user_init_deepRetn -------------------
_attribute_ram_code_
void user_init_deepRetn(void) {//after sleep this will get executed
	blc_ll_initBasicMCU();
	rf_set_power_level_index(cfg.rf_tx_power);
	blc_ll_recoverDeepRetention();
	bls_ota_registerStartCmdCb(app_enter_ota_mode);
}

//=========================================================
//----------------------- main_loop() ---------------------
_attribute_ram_code_
void main_loop(void) {
	blt_sdk_main_loop();
	while (clock_time() -  utc_time_sec_tick > utc_time_tick_step) {
		utc_time_sec_tick += utc_time_tick_step;
		utc_time_sec++; // + 1 sec
	}
	if (ota_is_working) {
#if USE_EXT_OTA
		if(ota_is_working == OTA_EXTENDED) {
			bls_pm_setManualLatency(3);
			clear_ota_area();
		} else
#endif
		{
			if ((ble_connected & BIT(CONNECTED_FLG_PAR_UPDATE))==0)
				bls_pm_setManualLatency(0);
			bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
		}
	} else {
		uint32_t new = clock_time();
		if(get_key_pressed()) {
			if(!key.key_pressed) {
				// key On event
				key.key_pressed = 1;
				key.key_pressed_tik = new;
				set_adv_con_time(0); // set connection adv.
				gpio_setup_up_down_resistor(GPIO_LED, PM_PIN_PULLUP_10K);
			} else {
				if(new - key.key_pressed_tik > 15*CLOCK_16M_SYS_TIMER_CLK_1S) {
					set_default_cfg();
				}
			}
		} else {
			 // key off
			if(key.key_pressed)
				gpio_setup_up_down_resistor(GPIO_LED, PM_PIN_PULLDOWN_100K);
			key.key_pressed = 0;
			key.key_pressed_tik = new;
		}
		if(key.rest_adv_int_tad < -80) {
			set_adv_con_time(1); // restore default adv.
		}

		if (!wrk_measure) {
			if (start_measure
//				&& sensor_i2c_addr
				&&	bls_pm_getSystemWakeupTick() - new > SENSOR_MEASURING_TIMEOUT + 5*CLOCK_16M_SYS_TIMER_CLK_1MS) {

				wrk_measure = 1;
				start_measure = 0;
				// no callback, data read sensor is next cycle
				WakeupLowPowerCb(0);
				//blt_sdk_main_loop();
				check_battery();
			} else {
				if (ble_connected && blc_ll_getTxFifoNumber() < 9) {  
					// connect, TxFifo ready
					if (end_measure) {
						end_measure = 0;
						if (RxTxValueInCCC && tx_measures) {
							if (tx_measures != 0xff)
								tx_measures--;
							ble_send_measures();
						}
						if (batteryValueInCCC)
							ble_send_battery();
						if (tempValueInCCC)
							ble_send_temp();
						if (temp2ValueInCCC)
							ble_send_temp2();
						if (humiValueInCCC)
							ble_send_humi();
#if USE_FLASH_MEMO
					} else if (rd_memo.cnt) {
						send_memo_blk();
#endif
					}
				}
				if (new - tim_measure >= measurement_step_time) {
					tim_measure = new;
					start_measure = 1;
				}
			}
		}
		bls_pm_setSuspendMask(
				SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
	}
}
