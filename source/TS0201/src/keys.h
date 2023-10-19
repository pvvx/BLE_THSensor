/*
 * key.h
 *
 *  Created on: 18.10.2023
 *      Author: pvvx
 */

#ifndef __KEYS_H_
#define __KEYS_H_

#include "app_config.h"

#define EXT_ADV_INTERVAL ADV_INTERVAL_50MS
#define EXT_ADV_COUNT 4

inline uint8_t get_key_pressed(void) {
	return BM_IS_SET(reg_gpio_in(GPIO_KEY), GPIO_KEY & 0xff)? 0 : 1;
}

typedef struct _key_wrk_t {
	int32_t rest_adv_int_tad;	// timer event restore adv.intervals (in adv count)
	uint32_t key_pressed_tik;   // timer1 key_pressed (in sys tik)
	uint8_t key_pressed;
} key_wrk_t;

extern key_wrk_t key;		// key switch work struct

#endif /* __KEYS_H_ */
