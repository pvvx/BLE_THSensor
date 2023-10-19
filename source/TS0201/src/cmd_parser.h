#pragma once 
enum {
	CMD_ID_DNAME    = 0x01, // Get/Set device name, "\0" - default: ATC_xxxx
	CMD_ID_GDEVS 	= 0x02, // Get address devises
	CMD_ID_I2C_SCAN = 0x03, // I2C scan
	CMD_ID_I2C_UTR	= 0x04, // Universal I2C/SMBUS read-write
	CMD_ID_SEN_ID	= 0x05,	// Get sensor ID
	CMD_ID_DEV_MAC	= 0x10, // Get/Set MAC [+RandMAC], [size][mac[6][randmac[2]]]
	CMD_ID_BKEY		= 0x18, // Get/set beacon bindkey in EEP
	CMD_ID_COMFORT  = 0x20, // Get/set comfort parameters
	CMD_ID_EXTDATA  = 0x22, // Get/set show ext. data
	CMD_ID_UTC_TIME = 0x23, // Get/set utc time (if USE_CLOCK = 1)
	CMD_ID_TADJUST  = 0x24, // Get/set adjust time clock delta (in 1/16 us for 1 sec)
	CMD_ID_MEASURE  = 0x33, // Start/stop notify measures in connection mode
	CMD_ID_LOGGER   = 0x35, // Read memory measures
	CMD_ID_CLRLOG	= 0x36, // Clear memory measures
	CMD_ID_CFG      = 0x55,	// Get/set device config
	CMD_ID_CFG_DEF  = 0x56,	// Set default device config
	CMD_ID_CFG_NS   = 0x5A,	// Get/set device config (not save to Flash)
	CMD_ID_LCD_DUMP = 0x60, // Get/set lcd buf
	CMD_ID_LCD_FLG  = 0x61, // Start/stop notify lcd dump and ...
	CMD_ID_PINCODE  = 0x70, // Set new PinCode 0..999999
	CMD_ID_MTU		= 0x71, // Request Mtu Size Exchange (23..255)
	CMD_ID_REBOOT	= 0x72, // Set Reboot on disconnect
	CMD_ID_SET_OTA	= 0x73, // Extension BigOTA: Get/set address and size OTA, erase sectors

	// Debug commands (unsupported in different versions!):
	CMD_ID_EEP_RW	= 0xDC,	// Get/set EEP
	CMD_ID_LR_RESET = 0xDD,	// Reset Long Range
	CMD_ID_DEBUG    = 0xDE // Test/Debug

} CMD_ID_KEYS;

#if USE_EXT_OTA  // Compatible BigOTA
	void clear_ota_area(void);
#endif

void cmd_parser(void * p);
