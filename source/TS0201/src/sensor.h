#pragma once
#include <stdint.h>

//==================================== CHT8305
// Timing
#define SENSOR_POWER_TIMEOUT_ms  	5
#define SENSOR_RESET_TIMEOUT_ms		5
#define SENSOR_MEASURING_TIMEOUT_ms	7
#define SENSOR_MEASURING_TIMEOUT  (SENSOR_MEASURING_TIMEOUT_ms * CLOCK_16M_SYS_TIMER_CLK_1MS) // clk tick


typedef struct _thsensor_coef_t {
	uint32_t temp_k;
	uint32_t humi_k;
	int16_t temp_z;
	int16_t humi_z;
} thsensor_coef_t;

typedef struct _thsensor_cfg_t {
	thsensor_coef_t coef;
	uint32_t id;
	uint8_t i2c_addr;
} thsensor_cfg_t;

extern thsensor_cfg_t thsensor;
#define thsensor_send_size 17

void init_sensor(void);
int read_sensor(void);


