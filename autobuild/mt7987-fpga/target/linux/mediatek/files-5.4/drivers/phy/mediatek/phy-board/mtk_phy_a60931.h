/* SPDX-License-Identifier: GPL-2.0 */
/*
 * MediaTek PCIe phy board controller driver.
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Chuanjia Liu <chuanjia.liu@mediatek.com>
 */

#ifndef __MTK_PHY_MTK1152_H__
#define __MTK_PHY_MTK1152_H__

void mtk_phy_base_init(unsigned int lane);
void mtk_phy_switch_lane(unsigned int lane);
int mtk_phy_set_port(const char *full_name);
void mtk_phy_write_phase(unsigned char phase);
unsigned char mtk_phy_read_phase(void);
int mtk_phy_init(unsigned int addr, unsigned int dir_offset,
			unsigned int dout_offset, unsigned int din_offset,
			unsigned int sda, unsigned int scl, unsigned int lane, const char *full_name);
int mtk_phy_exit(const char *full_name);

#endif
