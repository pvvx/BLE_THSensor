/********************************************************************************************************
 * @file     blt_common.c
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     Sep. 18, 2018
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *			 The information contained herein is confidential and proprietary property of Telink
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in.
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/

#include "tl_common.h"
#include "drivers.h"
// CGDK2 076000:  5A 58 2D 34 12 D7 F8 58 2D 34 12 D7 F8 FF FF FF = 58:2D:34:12:D7:F8
/*
 * VVWWXX38C1A4YYZZ
 * public_mac: 		  VVWWXX 38C1A4
 * random_static_mac: VVWWXXYYZZ C0
 */
_attribute_ram_code_
__attribute__((optimize("-Os")))
void SwapMacAddress(u8 *mac_out, u8 *mac_in) {
	mac_out[0] = mac_in[5];
	mac_out[1] = mac_in[4];
	mac_out[2] = mac_in[3];
	mac_out[3] = mac_in[2];
	mac_out[4] = mac_in[1];
	mac_out[5] = mac_in[0];
}

/* Erase Flash sector CFG_ADR_MAC
 * Save CUST_CAP_INFO_ADDR (Customize freq_offset adjust cap value) */
void flash_erase_mac_sector(u32 faddr) {
	u8 buf[32];
	flash_read_page(faddr+0xfe0, sizeof(buf), (unsigned char *) &buf);
	flash_erase_sector(faddr);
	flash_write(faddr+0xfe0, sizeof(buf), (unsigned char *)&buf);
}

__attribute__((optimize("-Os")))
void blc_newMacAddress(int flash_addr, u8 *mac_pub, u8 *mac_rand) {
	u8 mac_flash[8];
	flash_erase_mac_sector(flash_addr);
	memcpy(&mac_flash, mac_pub, 6);
	mac_flash[6] = mac_rand[3];
	mac_flash[7] = mac_rand[4];
	flash_write_page(flash_addr, sizeof(mac_flash), mac_flash);
}

__attribute__((optimize("-Os")))
void blc_initMacAddress(int flash_addr, u8 *mac_pub, u8 *mac_rand) {
	u8 mac_read[8];
	u32 * p = (u32 *) &mac_read;
	flash_read_page(flash_addr, sizeof(mac_read), mac_read);

	if (p[0] == 0xffffffff && p[1] == 0xffffffff) {
		// no public address on flash
		generateRandomNum(3, mac_read);
		mac_read[3] = 0x38;             //company id: 0xA4C138
		mac_read[4] = 0xC1;
		mac_read[5] = 0xA4;
		generateRandomNum(2, &mac_read[6]);
		flash_write_page(flash_addr, sizeof(mac_read), mac_read);
	}
	// copy public address
	memcpy(mac_pub, mac_read, 6);
	// copy random address
	mac_rand[0] = mac_pub[0];
	mac_rand[1] = mac_pub[1];
	mac_rand[2] = mac_pub[2];
	mac_rand[3] = mac_read[6];
	mac_rand[4] = mac_read[7];
	mac_rand[5] = 0xC0; 			//for random static
}

