/*
 * ext_ota.c
 *
 *  Created on: 04.03.2023
 *      Author: pvvx
 */
#include <stdint.h>
#include "tl_common.h"

#include "stack/ble/ble.h"
#include "vendor/common/blt_common.h"
#include "stack/ble/service/ble_ll_ota.h"
#include "ble.h"
#include "logger.h"
#include "flash_eep.h"
#include "cmd_parser.h"
#include "ext_ota.h"

#if USE_EXT_OTA  // Compatible BigOTA

#define ID_BOOTABLE 0x544c4e4b


#define OTA2_FADDR_ID (OTA2_FADDR + 8)
#define BIG_OTA2_FADDR 0x40000 // Big OTA2
#define BIG_OTA2_FADDR_ID (BIG_OTA2_FADDR + 8)

RAM ext_ota_t ext_ota;

extern unsigned char *_icload_size_div_16_;
extern unsigned char *_bin_size_;

// Current OTA header:
static const uint32_t head_id[4] = {
			0x00008026, // asm("tj __reset")
			0x025d0000, // id OTA ver
			ID_BOOTABLE, // id "bootable" = "KNLT"
			(uint32_t)(&_icload_size_div_16_ ) + 0x00880000 };


/* Reformat Big OTA to Low OTA */
void test_first_ota(void) {
	// find the real FW flash address
	uint32_t buf_blk[64], id, size, faddrw = OTA2_FADDR, faddrr = BIG_OTA2_FADDR;
	flash_unlock();
	flash_read_page(faddrr, 0x20, (unsigned char *) &buf_blk);
#if USE_FLASH_MEMO
	if(buf_blk[0] == MEMO_SEC_ID)
		return;
#endif
	if(memcmp(&buf_blk, &head_id, sizeof(head_id)) == 0) {
		// calculate size OTA
		size = (uint32_t)(&_bin_size_);
		size += 15;
		size &= ~15;
		size += 4;
		if(buf_blk[6] == size) { // OTA bin size
//			bls_ota_clearNewFwDataArea();
			flash_erase_sector(faddrw); // 45 ms, 4 mA
			flash_read_page(faddrr, sizeof(buf_blk), (unsigned char *) &buf_blk);
			buf_blk[2] &= 0xffffffff; // clear id "bootable"
			faddrr += sizeof(buf_blk);
			flash_write_page(faddrw, sizeof(buf_blk), (unsigned char *) &buf_blk);
			size += faddrw;
			faddrw += sizeof(buf_blk);
			while(faddrw < size) {
				if((faddrw & (FLASH_SECTOR_SIZE - 1)) == 0)
					flash_erase_sector(faddrw); // 45 ms, 4 mA
				// rd-wr 4kB - 20 ms, 4 mA
				flash_read_page(faddrr, sizeof(buf_blk), (unsigned char *) &buf_blk);
				faddrr += sizeof(buf_blk);
				flash_write_page(faddrw, sizeof(buf_blk), (unsigned char *) &buf_blk);
				faddrw += sizeof(buf_blk);
			}
			// set id "bootable" to new segment
			id = head_id[2]; // = "KNLT"
			flash_write_page(OTA2_FADDR_ID, sizeof(id), (unsigned char *) &id);
			// clear the "bootable" identifier on the current OTA segment
			id = 0;
			flash_write_page(BIG_OTA2_FADDR_ID, 1, (unsigned char *) &id);
			//flash_erase_sector(CFG_ADR_BIND); // Pair & Security info
			while(1)
				start_reboot();
		}
	}
}

uint32_t check_sector_clear(uint32_t addr) {
	uint32_t faddr = addr, efaddr, fbuf;
	faddr &= ~(FLASH_SECTOR_SIZE-1);
	efaddr = faddr + FLASH_SECTOR_SIZE;
	while(faddr < efaddr) {
		flash_read_page(faddr, sizeof(fbuf), (unsigned char *) &fbuf);
		if(fbuf != 0xffffffff) {
			flash_erase_sector(faddr & (~(FLASH_SECTOR_SIZE-1)));
			break;
		}
		faddr += 1024;
	}
	return efaddr;
}

void ota_result_cb(int result) {
	uint32_t boot_id;
	if(result == OTA_SUCCESS) {
		// clear the "bootable" identifier on the current work segment
		flash_read_page(OTA2_FADDR_ID, sizeof(boot_id), (unsigned char *) &boot_id);
		if(boot_id == ID_BOOTABLE) {
			boot_id = 0;
			flash_write_page(OTA2_FADDR_ID, 1, (unsigned char *) &boot_id);
		}
	}
}

uint8_t check_ext_ota(uint32_t ota_addr, uint32_t ota_size) {
	if(ota_is_working == OTA_EXTENDED)
		return EXT_OTA_BUSY;
	if(ota_is_working)
		return EXT_OTA_WORKS;
	if(ota_addr < 0x40000 && ota_size <= ota_firmware_size_k)
		return EXT_OTA_OK;
	if(ota_size >= 208
		|| ota_size < 4
		|| ota_addr & (FLASH_SECTOR_SIZE-1))
		return EXT_OTA_ERR_PARM;
	ble_connected |= BIT(CONNECTED_FLG_RESET_OF_DISCONNECT);
	ota_is_working = OTA_EXTENDED; // flag ext.ota
	ext_ota.start_addr = ota_addr;
	ext_ota.check_addr = ota_addr;
	ext_ota.ota_size = (ota_size + 3) & 0xfffffc;
	bls_ota_registerResultIndicateCb(ota_result_cb);
	bls_pm_setManualLatency(3);
	return EXT_OTA_BUSY;
}

void clear_ota_area(void) {
	union {
		uint8_t b[24];
		struct __attribute__((packed)) {
			uint16_t id_ok;
			uint32_t start_addr;
			uint32_t ota_size;
		} msg;
	} buf;
//	if(bls_pm_getSystemWakeupTick() - clock_time() < 512*CLOCK_16M_SYS_TIMER_CLK_1MS)
//		return;
	if(bls_ll_requestConnBrxEventDisable() < 256 || ext_ota.check_addr == 0)
		return;
	if (ext_ota.check_addr >= ext_ota.start_addr + (ext_ota.ota_size << 10)) {
			ota_firmware_size_k = ext_ota.ota_size;
			ota_program_offset = ext_ota.start_addr;
			buf.msg.id_ok = (EXT_OTA_READY << 8) + CMD_ID_SET_OTA;
			buf.msg.start_addr = ext_ota.start_addr;
			buf.msg.ota_size = ext_ota.ota_size;
			ota_is_working = OTA_WAIT; // flag ext.ota wait
			ext_ota.check_addr = 0;
	}
	else  {
			bls_ll_disableConnBrxEvent();
			ext_ota.check_addr = check_sector_clear(ext_ota.check_addr);
			bls_ll_restoreConnBrxEvent();
			buf.msg.id_ok = (EXT_OTA_EVENT << 8) + CMD_ID_SET_OTA;
			buf.msg.start_addr = ext_ota.check_addr;
			buf.msg.ota_size = 0;
	}
	bls_att_pushNotifyData(RxTx_CMD_OUT_DP_H, (u8 *)&buf.msg, sizeof(buf.msg));
	bls_pm_setSuspendMask(
			SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN); // MCU_STALL);
}

#endif // USE_EXT_OTA

