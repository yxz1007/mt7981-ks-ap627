#!/bin/bash

	echo "coverity make clean starts"
	echo "coverity make clean tops driver:"
	make package/mtk_soc/drivers/tops/clean V=s
	echo "coverity make clean eip_driver:"
	make package/mtk_soc/drivers/eip_driver/clean V=s
	echo "coverity make clean mt_wifi7 driver:"
	make package/mtk/drivers/mt_wifi7/clean V=s
	make package/mtk/drivers/mt_hwifi/clean V=s