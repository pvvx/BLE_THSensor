/*
 * custom_beacon.c
 *
 *  Created on: 07.03.2022
 *      Author: pvvx
 */
#include <stdint.h>
#include "tl_common.h"
#include "app_config.h"
#include "ble.h"
#include "app.h"
#include "keys.h"
#include "custom_beacon.h"
#include "ccm.h"

#if USE_SECURITY_BEACON

/* Encrypted atc/custom nonce */
typedef struct __attribute__((packed)) _enc_beacon_nonce_t{
    uint8_t  MAC[6];
    adv_cust_head_t head;
} enc_beacon_nonce_t;

__attribute__((optimize("-Os")))
void pvvx_encrypt_data_beacon(void) {
	padv_cust_enc_t p = (padv_cust_enc_t)&adv_buf.data;
	enc_beacon_nonce_t cbn;
	adv_cust_data_t data;
	uint8_t aad = 0x11;
	adv_buf.update_count = -1; // next call if next measured
	p->head.size = sizeof(adv_cust_enc_t) - 1;
	p->head.uid = GAP_ADTYPE_SERVICE_DATA_UUID_16BIT; // 16-bit UUID
	p->head.UUID = ADV_CUSTOM_UUID16; // GATT Service 0x181A Environmental Sensing (little-endian) (or 0x181C 'User Data'?)
	p->head.counter = (uint8_t)adv_buf.send_count;
	data.temp = measured_data.temp;
	data.humi = measured_data.humi;
	data.bat = measured_data.battery_level;
	data.trg = key.key_pressed;
	memcpy(cbn.MAC, mac_public, sizeof(cbn.MAC));
	memcpy(&cbn.head, p, sizeof(cbn.head));
	aes_ccm_encrypt_and_tag((const unsigned char *)&bindkey,
					   (uint8_t*)&cbn, sizeof(cbn),
					   &aad, sizeof(aad),
					   (uint8_t *)&data, sizeof(data),
					   (uint8_t *)&p->data,
					   p->mic, 4);
}

#endif // USE_SECURITY_BEACON

_attribute_ram_code_ __attribute__((optimize("-Os")))
void pvvx_data_beacon(void) {
	padv_custom_t p = (padv_custom_t)&adv_buf.data;
	memcpy(p->MAC, mac_public, 6);
	p->size = sizeof(adv_custom_t) - 1;
	p->uid = GAP_ADTYPE_SERVICE_DATA_UUID_16BIT; // 16-bit UUID
	p->UUID = ADV_CUSTOM_UUID16; // GATT Service 0x181A Environmental Sensing (little-endian)
	p->temperature = measured_data.temp; // x0.01 C
	p->humidity = measured_data.humi; // x0.01 %
#if USE_OUT_AVERAGE_BATTERY
	p->battery_mv = measured_data.average_battery_mv; // x mV
#else
	p->battery_mv = measured_data.battery_mv; // x mV
#endif
	p->battery_level = measured_data.battery_level; // x1 %
	p->counter = (uint8_t)adv_buf.send_count;
	p->flags = key.key_pressed;
}

typedef struct __attribute__((packed)) _ext_adv_cnt_t {
	uint8_t		size;	// = 6
	uint8_t		uid;	// = 0x16, 16-bit UUID https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/
	uint16_t	UUID;	// = 0x2AEB - Count 24
	uint8_t		cnt[3];
} ext_adv_cnt_t, * pext_adv_cnt_t;

typedef struct __attribute__((packed)) _ext_adv_digt_t {
	uint8_t		size;	// = 4
	uint8_t		uid;	// = 0x16, 16-bit UUID https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/
	uint16_t	UUID;	// = 0x2A56 - Digital State Bits
	uint8_t		bits;
} ext_adv_dig_t, * pext_adv_dig_t;

typedef struct __attribute__((packed)) _adv_event_t {
	ext_adv_dig_t dig;
	ext_adv_cnt_t cnt;
} adv_event_t, * padv_event_t;

void pvvx_event_beacon(void){
	pvvx_data_beacon();
	adv_buf.data_size = adv_buf.data[0] + 1;
}

#if USE_SECURITY_BEACON

void pvvx_encrypt_event_beacon(void){
	pvvx_encrypt_data_beacon();
	adv_buf.data_size = adv_buf.data[0] + 1;
}

#endif // USE_SECURITY_BEACON
