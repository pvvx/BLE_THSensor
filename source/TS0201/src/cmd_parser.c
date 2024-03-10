#include <stdint.h>
#include "tl_common.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_common.h"
#include "ble.h"
#include "i2c.h"
#include "sensor.h"
#include "app.h"
#include "flash_eep.h"
#include "keys.h"
#if USE_FLASH_MEMO
#include "logger.h"
#endif
#include "cmd_parser.h"
#if USE_EXT_OTA
#include "ext_ota.h"
#endif

#define _flash_read(faddr,len,pbuf) flash_read_page(FLASH_BASE_ADDR + (uint32_t)faddr, len, (uint8_t *)pbuf)

#define FLASH_MIMAC_ADDR CFG_ADR_MAC // 0x76000
//#define SEND_BUFFER_SIZE	 (ATT_MTU_SIZE-3) // = 20
//#define FLASH_SECTOR_SIZE 0x1000 // in "flash_eep.h"

#if USE_TIME_ADJUST
RAM uint32_t utc_set_time_sec; // clock setting time for delta calculation
#endif

__attribute__((optimize("-Os")))
void cmd_parser(void * p) {
	uint8_t send_buf[32];
	rf_packet_att_data_t *req = (rf_packet_att_data_t*) p;
	uint32_t len = req->l2cap - 3;
	if (len) {
		uint8_t cmd = req->dat[0];
		send_buf[0] = cmd;
		send_buf[1] = 0; // no err?
		uint32_t olen = 0;
		if (cmd == CMD_ID_DEV_ID) { // Get DEV_ID
			pdev_id_t p = (pdev_id_t) send_buf;
			// p->pid = CMD_ID_DEV_ID;
			// p->revision = 0;
			p->hw_version = DEVICE_TYPE + 0x100;
			p->sw_version = VERSION;
			p->dev_spec_data = 0;
			p->services = 0x01fff;
			olen = sizeof(dev_id_t);
		} else if (cmd == CMD_ID_MEASURE) { // Start/stop notify measures in connection mode
			if(len >= 2)
				tx_measures = req->dat[1];
			else {
				end_measure = 1;
				tx_measures = 1;
			}
			olen = 2;
		} else if (cmd == CMD_ID_CFG) { // Get/set config
			u8 tmp = ((volatile u8 *)&cfg.flg2)[0];
			if (--len > sizeof(cfg)) len = sizeof(cfg);
			if (len) {
				memcpy(&cfg, &req->dat[1], len);
			}
			test_config();
			ev_adv_timeout(0, 0, 0);
			if(tmp & MASK_FLG2_REBOOT) { // (cfg.flg2.bt5phy || cfg.flg2.ext_adv)
				ble_connected |= BIT(CONNECTED_FLG_RESET_OF_DISCONNECT); // reset device on disconnect
			}
			flash_write_cfg(&cfg, EEP_ID_CFG, sizeof(cfg));
			ble_send_cfg();
		} else if (cmd == CMD_ID_CFG_DEF) { // Set default config
			u8 tmp = ((volatile u8 *)&cfg.flg2)[0];
			memcpy(&cfg, &def_cfg, sizeof(cfg));
			test_config();
			if(tmp & MASK_FLG2_REBOOT) { // (cfg.flg2.bt5phy || cfg.flg2.ext_adv)
				ble_connected |= BIT(CONNECTED_FLG_RESET_OF_DISCONNECT); // reset device on disconnect
			}
			ev_adv_timeout(0, 0, 0);
			flash_write_cfg(&cfg, EEP_ID_CFG, sizeof(cfg));
			ble_send_cfg();
		} else if (cmd == CMD_ID_DEV_MAC) { // Get/Set mac
			if (len == 2 && req->dat[1] == 0) { // default MAC
				flash_erase_mac_sector(FLASH_MIMAC_ADDR);
				blc_initMacAddress(FLASH_MIMAC_ADDR, mac_public, mac_random_static);
				ble_connected |= BIT(CONNECTED_FLG_RESET_OF_DISCONNECT); // reset device on disconnect
			} else if (len == sizeof(mac_public)+2 && req->dat[1] == sizeof(mac_public)) {
				if (memcmp(mac_public, &req->dat[2], sizeof(mac_public))) {
					memcpy(mac_public, &req->dat[2], sizeof(mac_public));
					mac_random_static[0] = mac_public[0];
					mac_random_static[1] = mac_public[1];
					mac_random_static[2] = mac_public[2];
					generateRandomNum(2, &mac_random_static[3]);
					mac_random_static[5] = 0xC0; 			//for random static
					blc_newMacAddress(FLASH_MIMAC_ADDR, mac_public, mac_random_static);
					ble_connected |= BIT(CONNECTED_FLG_RESET_OF_DISCONNECT); // reset device on disconnect
				}
			} else	if (len == sizeof(mac_public)+2+2 && req->dat[1] == sizeof(mac_public)+2) {
				if (memcmp(mac_public, &req->dat[2], sizeof(mac_public))
						|| mac_random_static[3] != req->dat[2+6]
						|| mac_random_static[4] != req->dat[2+7] ) {
					memcpy(mac_public, &req->dat[2], sizeof(mac_public));
					mac_random_static[0] = mac_public[0];
					mac_random_static[1] = mac_public[1];
					mac_random_static[2] = mac_public[2];
					mac_random_static[3] = req->dat[2+6];
					mac_random_static[4] = req->dat[2+7];
					mac_random_static[5] = 0xC0; 			//for random static
					blc_newMacAddress(FLASH_MIMAC_ADDR, mac_public, mac_random_static);
					ble_connected |= BIT(CONNECTED_FLG_RESET_OF_DISCONNECT); // reset device on disconnect
				}
			}
			send_buf[1] = 8;
			_flash_read(FLASH_MIMAC_ADDR, 8, &send_buf[2]); // MAC[6] + mac_random[2]
			olen = 8 + 2;
#if USE_SECURITY_BEACON
		} else if (cmd == CMD_ID_BKEY) { // Get/set beacon bindkey
			if (len == sizeof(bindkey) + 1) {
				memcpy(bindkey, &req->dat[1], sizeof(bindkey));
				flash_write_cfg(&bindkey, EEP_ID_KEY, sizeof(bindkey));
				bindkey_init();
			}
			if (flash_read_cfg(&bindkey, EEP_ID_KEY, sizeof(bindkey)) == sizeof(bindkey)) {
				memcpy(&send_buf[1], bindkey, sizeof(bindkey));
				olen = sizeof(bindkey) + 1;
			} else { // No bindkey in EEP!
				send_buf[1] = 0xff;
				olen = 2;
			}
#endif
#if BLE_SECURITY_ENABLE
		} else if (cmd == CMD_ID_PINCODE && len > 4) { // Set new pinCode 0..999999
			uint32_t old_pincode = pincode;
			uint32_t new_pincode = req->dat[1] | (req->dat[2]<<8) | (req->dat[3]<<16) | (req->dat[4]<<24);
			if (pincode != new_pincode) {
				pincode = new_pincode;
				if (flash_write_cfg(&pincode, EEP_ID_PCD, sizeof(pincode))) {
					if ((pincode != 0) ^ (old_pincode != 0)) {
						bls_smp_eraseAllParingInformation();
						ble_connected |= BIT(CONNECTED_FLG_RESET_OF_DISCONNECT); // reset device on disconnect
					}
					send_buf[1] = 1;
				} else	send_buf[1] = 3;
			} //else send_buf[1] = 0;
			olen = 2;
#endif
		} else if (cmd == CMD_ID_DNAME) { // Get/Set device name
			if (--len > MAX_DEV_NAME_LEN) len = MAX_DEV_NAME_LEN;
			if (len) {
				flash_write_cfg(&req->dat[1], EEP_ID_DVN, (req->dat[1] != 0)? len : 0);
				ble_set_name();
				ble_connected |= BIT(CONNECTED_FLG_RESET_OF_DISCONNECT); // reset device on disconnect
			}
			memcpy(&send_buf[1], &ble_name[2], ble_name[0] - 1);
			olen = ble_name[0];
		} else if (cmd == CMD_ID_UTC_TIME) { // Get/set utc time
			if (--len > sizeof(utc_time_sec)) len = sizeof(utc_time_sec);
			if (len) {
				memcpy(&utc_time_sec, &req->dat[1], len);
#if USE_TIME_ADJUST
				utc_set_time_sec = utc_time_sec;
#endif
			}
			memcpy(&send_buf[1], &utc_time_sec, sizeof(utc_time_sec));
#if USE_TIME_ADJUST
			memcpy(&send_buf[sizeof(utc_time_sec) + 1], &utc_set_time_sec, sizeof(utc_set_time_sec));
			olen = sizeof(utc_time_sec) + sizeof(utc_set_time_sec) + 1;
#else
			olen = sizeof(utc_time_sec) + 1;
#endif
#if USE_TIME_ADJUST
		} else if (cmd == CMD_ID_TADJUST) { // Get/set adjust time clock delta (in 1/16 us for 1 sec)
			if (len > 2) {
				int16_t delta = req->dat[1] | (req->dat[2] << 8);
				utc_time_tick_step = CLOCK_16M_SYS_TIMER_CLK_1S + delta;
				flash_write_cfg(&utc_time_tick_step, EEP_ID_TIM, sizeof(utc_time_tick_step));
			}
			memcpy(&send_buf[1], &utc_time_tick_step, sizeof(utc_time_tick_step));
			olen = sizeof(utc_time_tick_step) + 1;
#endif
#if USE_FLASH_MEMO
		} else if (cmd == CMD_ID_LOGGER && len > 2) { // Read memory measures
			rd_memo.cnt = req->dat[1] | (req->dat[2] << 8);
			if (rd_memo.cnt) {
				rd_memo.saved = memo;
				if (len > 4)
					rd_memo.cur = req->dat[3] | (req->dat[4] << 8);
				else
					rd_memo.cur = 0;
				bls_pm_setManualLatency(0);
			} else
				bls_pm_setManualLatency(cfg.connect_latency);
		} else if (cmd == CMD_ID_CLRLOG && len > 2) { // Clear memory measures
			if (req->dat[1] == 0x12 && req->dat[2] == 0x34) {
				clear_memo();
				olen = 2;
			}
#endif
		} else if (cmd == CMD_ID_MTU && len > 1) { // Request Mtu Size Exchange
			if (req->dat[1] >= ATT_MTU_SIZE)
				send_buf[1] = blc_att_requestMtuSizeExchange(BLS_CONN_HANDLE, req->dat[1]);
			else
				send_buf[1] = 0xff;
			olen = 2;
		} else if (cmd == CMD_ID_REBOOT) { // Set Reboot on disconnect
			ble_connected |= BIT(CONNECTED_FLG_RESET_OF_DISCONNECT); // reset device on disconnect
			olen = 2;
		} else if (cmd == CMD_ID_SET_OTA) { // Set OTA address and size
#if USE_EXT_OTA  // Compatible BigOTA
			uint32_t ota_addr, ota_size;
			if (len > 8) {
				memcpy(&ota_addr, &req->dat[1], 4);
				memcpy(&ota_size, &req->dat[5], 4);
				send_buf[1] = check_ext_ota(ota_addr, ota_size);
			} // else send_buf[1] = 0;
#endif
			memcpy(&send_buf[2], &ota_program_offset, 4);
			memcpy(&send_buf[2+4], &ota_firmware_size_k, 4);
			olen = 2 + 8;
		} else if (cmd == CMD_ID_GDEVS) {   // Get address devises
			send_buf[1] = thsensor.i2c_addr;
			olen = 1 + 1;
		} else if (cmd == CMD_ID_I2C_SCAN) {   // Universal I2C/SMBUS read-write
			len = 0;
			olen = 1;
			while(len < 0x100 && olen < SEND_BUFFER_SIZE) {
				send_buf[olen] = (uint8_t)scan_i2c_addr(len);
				if(send_buf[olen])
					olen++;
				len += 2;
			}
		} else if (cmd == CMD_ID_I2C_UTR) {   // Universal I2C/SMBUS read-write
			i2c_utr_t * pbufi = (i2c_utr_t *)&req->dat[1];
			olen = pbufi->rdlen & 0x7f;
			if(len > sizeof(i2c_utr_t)
				&& olen <= SEND_BUFFER_SIZE - 3 // = 17
				&& I2CBusUtr(&send_buf[3],
						pbufi,
						len - sizeof(i2c_utr_t) - 1) == 0 // wrlen: - addr
						)  {
				send_buf[1] = len - 1 - sizeof(i2c_utr_t); // write data len
				send_buf[2] = pbufi->wrdata[0]; // i2c addr
				olen += 3;
			} else {
				send_buf[1] = 0xff; // Error cmd
				olen = 2;
			}
#if (DEV_SERVICES & SERVICE_THS)
		} else if (cmd == CMD_ID_CFS) {	// Get/Set sensor config
			if (--len > sizeof(thsensor.coef))
				len = sizeof(thsensor.coef);
			if (len) {
				memcpy(&thsensor.coef, &req->dat[1], len);
				flash_write_cfg(&thsensor.coef, EEP_ID_CFS, sizeof(thsensor.coef));
			}
			memcpy(&send_buf[1], &thsensor, thsensor_send_size);
			olen = thsensor_send_size + 1;
		} else if (cmd == CMD_ID_CFS_DEF) {	// Get/Set default sensor config
			memset(&thsensor, 0, thsensor_send_size);
			init_sensor();
			memcpy(&send_buf[1], &thsensor, thsensor_send_size);
			olen = thsensor_send_size + 1;
		} else if (cmd == CMD_ID_SEN_ID) { // Get sensor ID
			memcpy(&send_buf[1], &thsensor.id, sizeof(thsensor.id));
			olen = sizeof(thsensor.id) + 1;
#endif
			// Debug commands (unsupported in different versions!):
		} else if (cmd == CMD_ID_EEP_RW && len > 2) {
			send_buf[1] = req->dat[1];
			send_buf[2] = req->dat[2];
			olen = req->dat[1] | (req->dat[2] << 8);
			if(len > 3) {
				flash_write_cfg(&req->dat[3], olen, len - 3);
			}
			int16_t i = flash_read_cfg(&send_buf[3], olen, SEND_BUFFER_SIZE - 3);
			if(i < 0) {
				send_buf[1] = (uint8_t)(i & 0xff); // Error
				olen = 2;
			} else
				olen = i + 3;
		} else if (cmd == CMD_ID_DEBUG && len > 3) { // test/debug
			_flash_read((req->dat[1] | (req->dat[2]<<8) | (req->dat[3]<<16)), 18, &send_buf[4]);
			memcpy(send_buf, &req->dat, 4);
			olen = 18+4;
		} else if (cmd == CMD_ID_LR_RESET) { // Reset Long Range
			cfg.flg2.longrange = 0;
			cfg.flg2.bt5phy = 0;
			flash_write_cfg(&cfg, EEP_ID_CFG, sizeof(cfg));
			ble_send_cfg();
			ble_connected |= BIT(CONNECTED_FLG_RESET_OF_DISCONNECT); // reset device on disconnect

		} else {
			send_buf[1] = 0xff; // Error cmd
			olen = 2;
		}
		if (olen)
			bls_att_pushNotifyData(RxTx_CMD_OUT_DP_H, send_buf, olen);
	}
}
