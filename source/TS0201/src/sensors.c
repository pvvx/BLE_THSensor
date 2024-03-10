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

RAM  thsensor_cfg_t thsensor;

//==================================== SHT30

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

//==================================== CHT8305

//  I2C addres
#define CHT8305_I2C_ADDR		0x40
#define CHT8305_I2C_ADDR_MAX	0x43

//  Registers
#define CHT8305_REG_TMP		0x00
#define CHT8305_REG_HMD		0x01
#define CHT8305_REG_CFG		0x02
#define CHT8305_REG_ALR		0x03
#define CHT8305_REG_VLT		0x04
#define CHT8305_REG_MID		0xfe
#define CHT8305_REG_VID		0xff

//  Config register mask
#define CHT8305_CFG_SOFT_RESET          0x8000
#define CHT8305_CFG_CLOCK_STRETCH       0x4000
#define CHT8305_CFG_HEATER              0x2000
#define CHT8305_CFG_MODE                0x1000
#define CHT8305_CFG_VCCS                0x0800
#define CHT8305_CFG_TEMP_RES            0x0400
#define CHT8305_CFG_HUMI_RES            0x0300
#define CHT8305_CFG_ALERT_MODE          0x00C0
#define CHT8305_CFG_ALERT_PENDING       0x0020
#define CHT8305_CFG_ALERT_HUMI          0x0010
#define CHT8305_CFG_ALERT_TEMP          0x0008
#define CHT8305_CFG_VCC_ENABLE          0x0004
#define CHT8305_CFG_VCC_RESERVED        0x0003

struct __attribute__((packed)) _cht8305_config_t{
	uint16_t reserved 	: 2;
	uint16_t vccen		: 1;
	uint16_t talt		: 1;
	uint16_t halt 		: 1;
	uint16_t aps 		: 1;
	uint16_t altm		: 2;
	uint16_t h_res 		: 2;
	uint16_t t_res		: 1;
	uint16_t vccs		: 1;
	uint16_t mode		: 1;
	uint16_t heater		: 1;
	uint16_t clkstr		: 1;
	uint16_t srst		: 1;
} cht8305_config_t;

#define CRC_POLYNOMIAL  0x131 // P(x) = x^8 + x^5 + x^4 + 1 = 100110001

const thsensor_coef_t def_thcoef_sht30 = {
		.temp_k = 17500,
		.humi_k = 20000,
		.temp_z = -4500,
		.humi_z = 0
};

const thsensor_coef_t def_thcoef_cht8305 = {
		.temp_k = 16500,
		.humi_k = 10000,
		.temp_z = -4000,
		.humi_z = 0
};

static _attribute_ram_code_
void send_sensor_word(uint16_t cmd) {
	if ((reg_clk_en0 & FLD_CLK0_I2C_EN)==0)
			init_i2c();
	reg_i2c_id = thsensor.i2c_addr;
	reg_i2c_adr_dat = cmd;
	reg_i2c_ctrl = FLD_I2C_CMD_START | FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_DO | FLD_I2C_CMD_STOP;
	while (reg_i2c_status & FLD_I2C_CMD_BUSY);
}

_attribute_ram_code_
static uint8_t sensor_crc(uint8_t crc) {
	int i;
	for(i = 8; i > 0; i--) {
		if (crc & 0x80)
			crc = (crc << 1) ^ (CRC_POLYNOMIAL & 0xff);
		else
			crc = (crc << 1);
	}
	return crc;
}

static void check_sensor(void) {
	uint8_t buf[4];
	int test_i2c_addr = CHT8305_I2C_ADDR << 1;
	do {
		if ((thsensor.i2c_addr = (uint8_t) scan_i2c_addr(test_i2c_addr)) != 0) {
			if(thsensor.i2c_addr >= (CHT8305_I2C_ADDR << 1) && thsensor.i2c_addr <= (CHT8305_I2C_ADDR_MAX << 1)) {
				read_i2c_byte_addr(thsensor.i2c_addr, CHT8305_REG_MID, buf, 2);
				thsensor.id = (buf[0] << 24) | (buf[1] << 16);
				read_i2c_byte_addr(thsensor.i2c_addr, CHT8305_REG_VID, buf, 2);
				thsensor.id |= (buf[0] << 8) | buf[1];
				// Soft reset command
				buf[0] = CHT8305_REG_CFG;
				buf[1] = (CHT8305_CFG_SOFT_RESET | CHT8305_CFG_CLOCK_STRETCH | CHT8305_CFG_MODE) >> 8;
				buf[2] = (CHT8305_CFG_SOFT_RESET | CHT8305_CFG_CLOCK_STRETCH | CHT8305_CFG_MODE) & 0xff;
				send_i2c_buf(thsensor.i2c_addr, buf, 3);
				pm_wait_ms(SENSOR_RESET_TIMEOUT_ms);
				// Configure
				buf[0] = CHT8305_REG_CFG;
				buf[1] = (CHT8305_CFG_CLOCK_STRETCH | CHT8305_CFG_MODE) >> 8;
				buf[2] = (CHT8305_CFG_CLOCK_STRETCH | CHT8305_CFG_MODE) & 0xff;
				send_i2c_buf(thsensor.i2c_addr, buf, 3);
				pm_wait_ms(SENSOR_RESET_TIMEOUT_ms);

				send_i2c_byte(thsensor.i2c_addr, CHT8305_REG_TMP); // start measure T/H
				memcpy(&thsensor.coef, &def_thcoef_cht8305, sizeof(thsensor.coef));
				pm_wait_ms(SENSOR_MEASURING_TIMEOUT_ms);
				read_sensor();
			} else if (thsensor.i2c_addr == (SHT30_I2C_ADDR_A<<1) || thsensor.i2c_addr == (SHT30_I2C_ADDR_B<<1)) {
				thsensor.id = (0x30<<16) | thsensor.i2c_addr;
				send_sensor_word(SHT30_SOFT_RESET); // Soft reset command
				memcpy(&thsensor.coef, &def_thcoef_sht30, sizeof(thsensor.coef));
				sleep_us(SHT30_SOFT_RESET_us);
				send_sensor_word(SHT30_HIMEASURE); // start measure T/H
			} else
				thsensor.i2c_addr = 0;
			break;
		}
		test_i2c_addr += 2;
	} while(test_i2c_addr <= (SHT30_I2C_ADDR_B << 1));
}

_attribute_ram_code_ __attribute__((optimize("-Os")))
static int read_sensor_cht8305(void) {
	uint32_t _temp, i = 3;
	uint8_t reg_data[4];
	while(i--) {
		if (read_i2c_bytes(thsensor.i2c_addr, reg_data, sizeof(reg_data)) == 0) {
			_temp = (reg_data[0] << 8) | reg_data[1];
			measured_data.temp = ((int32_t)(_temp * thsensor.coef.temp_k) >> 16) + thsensor.coef.temp_z; // x 0.01 C // 16500 -4000
			_temp = (reg_data[2] << 8) | reg_data[3];
			measured_data.humi = ((uint32_t)(_temp * thsensor.coef.humi_k) >> 16) + thsensor.coef.humi_z; // x 0.01 % // 10000 -0
			if (measured_data.humi < 0) measured_data.humi = 0;
			else if (measured_data.humi > 9999) measured_data.humi = 9999;
			send_i2c_byte(thsensor.i2c_addr, CHT8305_REG_TMP); // start measure T/H
			return 1;
		}
	}
	send_i2c_byte(thsensor.i2c_addr, CHT8305_REG_TMP); // start measure T/H
	return 0;
}

_attribute_ram_code_ __attribute__((optimize("-Os")))
static int read_sensor_sht30(void) {
	uint16_t _temp;
	uint16_t _humi;
	uint8_t data, crc; // calculated checksum
	int i;
	if ((reg_clk_en0 & FLD_CLK0_I2C_EN)==0)
		init_i2c();
	reg_i2c_id = thsensor.i2c_addr | FLD_I2C_WRITE_READ_BIT;
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
				measured_data.temp = ((int32_t)(_temp * thsensor.coef.temp_k) >> 16) + thsensor.coef.temp_z; // x 0.01 C //17500 - 4500
				measured_data.humi = ((uint32_t)(_humi * thsensor.coef.humi_k) >> 16) + thsensor.coef.humi_z; // x 0.01 %	   // 10000 -0
				if (measured_data.humi < 0) measured_data.humi = 0;
				else if (measured_data.humi > 9999) measured_data.humi = 9999;
				//measured_data.count++;
				send_sensor_word(SHT30_HIMEASURE); // start measure T/H
				return 1;
			}
		}
	} while (i--);
	return 0;
}


int read_sensor(void) {
	if (thsensor.i2c_addr == 0)
		check_sensor();
	else if (thsensor.i2c_addr >= (SHT30_I2C_ADDR_A<<1))
		return read_sensor_sht30();
	else
		return read_sensor_cht8305();
	return 0;
}


void init_sensor(void) {
	send_i2c_byte(0, 0x06); // Reset command using the general call address
	pm_wait_ms(SENSOR_POWER_TIMEOUT_ms);
	check_sensor();
}

