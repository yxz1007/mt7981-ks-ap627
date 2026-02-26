/* SPDX-License-Identifier: GPL-2.0 */
/*
 * MediaTek PCIe phy board controller driver.
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Chuanjia Liu <chuanjia.liu@mediatek.com>
 */

#ifndef __MTK_PCIE_GEN3_H__
#define __MTK_PCIE_GEN3_H__

#define PCIE_BASIC_STATUS		0x18

#define PCIE_SETTING_REG		0x80
#define PCIE_RC_MODE			BIT(0)
#define PCIE_2LANE_SUPPORT		BIT(8)
#define PCIE_4LANE_SUPPORT		BIT(9)
#define PCIE_8LANE_SUPPORT		BIT(10)
#define PCIE_16LANE_SUPPORT		BIT(11)
#define PCIE_GEN2_SUPPORT		BIT(12)
#define PCIE_GEN3_SUPPORT		BIT(13)
#define PCIE_GEN4_SUPPORT		BIT(14)

#define PCIE_RST_CTRL_V3	0x148
#define PCIE_MAC_RSTB_V3	BIT(0)
#define PCIE_PHY_RSTB_V3	BIT(1)
#define PCIE_BRG_RSTB_V3	BIT(2)
#define PCIE_PE_RSTB_V3		BIT(3)

#define PCIE_DATA_LINK_STATUS_V3	0x154
#define PCIE_DATA_LINKUP_V3			BIT(8)

int mtk_pcie_relink(void __iomem *port_base);
void mtk_pcie_set_max_speed(void __iomem *port_base, unsigned int speed);
void mtk_pcie_set_max_lane(void __iomem *port_base, unsigned int lane);
void mtk_pcie_assert_reset(void __iomem *port_base);
void mtk_pcie_get_current_info(void __iomem *port_base,
			unsigned int *speed, unsigned int *lane);
void mtk_pcie_init(void __iomem *port_base, unsigned int speed,
				unsigned int lane);

#endif
