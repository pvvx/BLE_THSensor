#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "vendor/common/user_config.h"
#include "app_config.h"
#include "drivers/8258/gpio_8258.h"
#include "drivers/8258/pm.h"

#include "i2c.h"
#include "sensor.h"
#include "app.h"

RAM uint8_t sensor_i2c_addr;
RAM uint32_t sensor_id;


void init_sensor(void) {
	uint8_t buf[4];
	int test_i2c_addr = CHT8305_I2C_ADDR << 1;
	sensor_id = 0;
	send_i2c_byte(0, 0x06); // Reset command using the general call address
	pm_wait_ms(SENSOR_POWER_TIMEOUT_ms);
	while(test_i2c_addr <= (CHT8305_I2C_ADDR_MAX << 1)) {
		sensor_i2c_addr = (uint8_t) scan_i2c_addr(test_i2c_addr);
		if(sensor_i2c_addr) {
			read_i2c_byte_addr(sensor_i2c_addr, CHT8305_REG_MID, buf, 2);
			sensor_id = (buf[0] << 24) | (buf[1] << 16);
			read_i2c_byte_addr(sensor_i2c_addr, CHT8305_REG_VID, buf, 2);
			sensor_id |= (buf[0] << 8) | buf[1];
			// Soft reset command
			buf[0] = CHT8305_REG_CFG;
			buf[1] = (CHT8305_CFG_SOFT_RESET | CHT8305_CFG_CLOCK_STRETCH | CHT8305_CFG_MODE) >> 8;
			buf[2] = (CHT8305_CFG_SOFT_RESET | CHT8305_CFG_CLOCK_STRETCH | CHT8305_CFG_MODE) & 0xff;
			send_i2c_buf(sensor_i2c_addr, buf, 3);
			pm_wait_ms(SENSOR_RESET_TIMEOUT_ms);
			// Configure
			buf[0] = CHT8305_REG_CFG;
			buf[1] = (CHT8305_CFG_CLOCK_STRETCH | CHT8305_CFG_MODE) >> 8;
			buf[2] = (CHT8305_CFG_CLOCK_STRETCH | CHT8305_CFG_MODE) & 0xff;
			send_i2c_buf(sensor_i2c_addr, buf, 3);
			pm_wait_ms(SENSOR_RESET_TIMEOUT_ms);

			send_i2c_byte(sensor_i2c_addr, CHT8305_REG_TMP); // start measure T/H
			pm_wait_ms(SENSOR_MEASURING_TIMEOUT_ms);
			read_sensor_cb();
			break;
		}
		test_i2c_addr += 2;
	}
}

_attribute_ram_code_ __attribute__((optimize("-Os"))) int read_sensor_cb(void) {
	uint32_t _temp, i = 3;
	uint8_t reg_data[4];
	if (sensor_i2c_addr != 0) {
		while(i--) {
			if (read_i2c_bytes(sensor_i2c_addr, reg_data, sizeof(reg_data)) == 0) {
				_temp = (reg_data[0] << 8) | reg_data[1];
				measured_data.temp = ((uint32_t)(_temp * 16500) >> 16) - 4000 + cfg.temp_offset * 10; // x 0.01 C
				_temp = (reg_data[2] << 8) | reg_data[3];
				measured_data.humi = ((uint32_t)(_temp * 10000) >> 16) + cfg.humi_offset * 10; // x 0.01 %
				if (measured_data.humi < 0) measured_data.humi = 0;
				else if (measured_data.humi > 9999) measured_data.humi = 9999;
				send_i2c_byte(sensor_i2c_addr, CHT8305_REG_TMP); // start measure T/H
				return 1;
			}
		}
		send_i2c_byte(sensor_i2c_addr, CHT8305_REG_TMP); // start measure T/H
	}
	return 0;
}


