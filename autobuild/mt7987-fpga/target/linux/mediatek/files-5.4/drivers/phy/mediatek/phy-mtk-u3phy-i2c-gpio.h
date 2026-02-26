/*
 * Copyright (c) 2013-2015, Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _U3PHY_IIC_GPIO_H_
#define _U3PHY_IIC_GPIO_H_

typedef unsigned char u8;
typedef unsigned int  u32;

int u3phy_write_reg(void __iomem *ioi2c, u8 dev_id, u8 address, u32 value);
u8  u3phy_read_reg(void __iomem *ioi2c, u8 dev_id,  u8 address);
int u3phy_write_reg32(void __iomem *ioi2c, u8 dev_id, u32 addr, u32 data);
u32 u3phy_read_reg32(void __iomem *ioi2c, u8 dev_id, u32 addr);
u32 u3phy_read_reg32(void __iomem *ioi2c, u8 dev_id, u32 addr);
int u3phy_write_reg8(void __iomem *ioi2c, u8 dev_id, u32 addr, u8 data);
u8  u3phy_read_reg8(void __iomem *ioi2c, u8 dev_id, u32 addr);
u32 u3phy_readlmsk(void __iomem *ioi2c, u8 i2c_addr,
				u32 reg_addr32, u32 offset, u32 mask);
int u3phy_writelmsk(void __iomem *ioi2c, u8 i2c_addr,
			u32 reg_addr32, u32 offset, u32 mask, u32 data);
#endif
