#!/bin/bash

	source ./autobuild/lede-build-sanity.sh

	echo "coverity make build starts"
	echo "coverity make build mt_wifi7 driver:"
	make package/mtk/drivers/mt_wifi7/compile KBUILD_MODPOST_WARN=1 V=s
	echo "coverity make build tops driver:"
	make package/mtk_soc/drivers/tops/compile V=s
	echo "coverity make build eip_driver:"
	make package/mtk_soc/drivers/eip_driver/compile V=s