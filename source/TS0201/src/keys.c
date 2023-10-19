/*
 * keys.c
 *
 *  Created on: 20.02.2022
 *      Author: pvvx
 */

#include <stdint.h>
#include "tl_common.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "drivers.h"
#include "keys.h"

RAM	key_wrk_t key;	// key switch work struct
