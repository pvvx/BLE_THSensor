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

#define SHT30_POWER_TIMEOUT_us	1500	// time us
#define SHT30_SOFT_RESET_us		1500	// time us
#define SHT30_HI_MEASURE_us		15500	// time us
#define SHT30_LO_MEASURE_us		4500	// time us

#define SHT30_I2C_ADDR_A	0x44
#define SHT30_I2C_ADDR_B	0x45
#define SHT30_SOFT_RESET	0xA230 // Soft reset command
#define SHT30_HIMEASURE		0x0024 // Measurement commands, Clock Stretching Disabled, Normal Mode, Read T First
#define SHT30_HIMEASURE_CS	0x062C // Measurement commands, Clock Stretching, Normal Mode, Read T First
#define SHT30_LPMEASURE		0x1624 // Measurement commands, Clock Stretching Disabled, Low Power Mode, Read T First
#define SHT30_LPMEASURE_CS	0x102C // Measurement commands, Clock Stretching, Low Power Mode, Read T First

#define CRC_POLYNOMIAL  0x131 // P(x) = x^8 + x^5 + x^4 + 1 = 100110001

#if DEVICE_TYPE	== DEVICE_TH03Z

static _attribute_ram_code_
void send_sensor_word(uint16_t cmd) {
	if ((reg_clk_en0 & FLD_CLK0_I2C_EN)==0)
			init_i2c();
	reg_i2c_id = sensor_i2c_addr;
	reg_i2c_adr_dat = cmd;
	reg_i2c_ctrl = FLD_I2C_CMD_START | FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_DO | FLD_I2C_CMD_STOP;
	while (reg_i2c_status & FLD_I2C_CMD_BUSY);
}

_attribute_ram_code_
uint8_t sensor_crc(uint8_t crc) {
	int i;
	for(i = 8; i > 0; i--) {
		if (crc & 0x80)
			crc = (crc << 1) ^ (CRC_POLYNOMIAL & 0xff);
		else
			crc = (crc << 1);
	}
	return crc;
}

void check_sensor(void) {
	sensor_i2c_addr = (uint8_t) scan_i2c_addr(SHT30_I2C_ADDR_A << 1);
	if(!sensor_i2c_addr)
		sensor_i2c_addr = (uint8_t) scan_i2c_addr(SHT30_I2C_ADDR_B << 1);
	if(sensor_i2c_addr) {
		sensor_id = (0x30<<16) | sensor_i2c_addr;
		send_sensor_word(SHT30_SOFT_RESET); // Soft reset command
		sleep_us(SHT30_SOFT_RESET_us);
		send_sensor_word(SHT30_HIMEASURE); // start measure T/H
	}
}

void init_sensor(void) {
	send_i2c_byte(0, 0x06); // Reset command using the general call address
	sleep_us(SHT30_POWER_TIMEOUT_us);
	check_sensor();
}

_attribute_ram_code_ __attribute__((optimize("-Os")))
int read_sensor_cb(void) {
	uint16_t _temp;
	uint16_t _humi;
	uint8_t data, crc; // calculated checksum
	int i;
	if ((reg_clk_en0 & FLD_CLK0_I2C_EN)==0)
		init_i2c();
	if (sensor_i2c_addr == 0) {
		check_sensor();
		return 0;
	}
	reg_i2c_id = sensor_i2c_addr | FLD_I2C_WRITE_READ_BIT;
	i = 256;
	do {
		reg_i2c_ctrl = FLD_I2C_CMD_ID | FLD_I2C_CMD_START;
		while (reg_i2c_status & FLD_I2C_CMD_BUSY);
		if (reg_i2c_status & FLD_I2C_NAK) {
			reg_i2c_ctrl = FLD_I2C_CMD_STOP;
			while (reg_i2c_status & FLD_I2C_CMD_BUSY);
		} else { // ACK ok
			reg_i2c_ctrl = FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID;
			while (reg_i2c_status & FLD_I2C_CMD_BUSY);
			data = reg_i2c_di;
			reg_i2c_ctrl = FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID;
			_temp = data << 8;
			crc = sensor_crc(data ^ 0xff);
			while (reg_i2c_status & FLD_I2C_CMD_BUSY);
			data = reg_i2c_di;
			reg_i2c_ctrl = FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID;
			_temp |= data;
			crc = sensor_crc(crc ^ data);
			while (reg_i2c_status & FLD_I2C_CMD_BUSY);
			data = reg_i2c_di;
			reg_i2c_ctrl = FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID;
			while (reg_i2c_status & FLD_I2C_CMD_BUSY);
			_humi = reg_i2c_di << 8;
			reg_i2c_ctrl = FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID | FLD_I2C_CMD_ACK;
			while (reg_i2c_status & FLD_I2C_CMD_BUSY);
			_humi |= reg_i2c_di;
			reg_i2c_ctrl = FLD_I2C_CMD_STOP;
			while (reg_i2c_status & FLD_I2C_CMD_BUSY);
			if (crc == data && _temp != 0xffff) {
				measured_data.temp = ((int32_t)(17500*_temp) >> 16) - 4500 + cfg.temp_offset * 10; // x 0.01 C
				measured_data.humi = ((uint32_t)(10000*_humi) >> 16) + cfg.humi_offset * 10; // x 0.01 %
				if (measured_data.humi < 0) measured_data.humi = 0;
				else if (measured_data.humi > 9999) measured_data.humi = 9999;
				//measured_data.count++;
				send_sensor_word(SHT30_HIMEASURE); // start measure T/H
				return 1;
			}
		}
	} while (i--);
	check_sensor();
	return 0;
}

#else

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

#endif
