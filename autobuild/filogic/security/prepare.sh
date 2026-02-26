#!/bin/bash

declare -A __sb_vars

__sb_vars["FIT_KEY_DIR"]="\$(TOPDIR)/../../keys"
__sb_vars["ROE_KEY_DIR"]="\$(TOPDIR)/../../keys"
__sb_vars["FIT_KEY_NAME"]="fit_key"
__sb_vars["ROE_KEY_NAME"]="roe_key"
__sb_vars["FIRMWARE_ENC_ALGO"]="tee_aes256"
__sb_vars["ANTI_ROLLBACK_TABLE"]="\$(TOPDIR)/../../fw_ar_table.xml"
__sb_vars["AUTO_AR_CONF"]="\$(TOPDIR)/../../auto_ar_conf.mk"
__sb_vars["HASHED_BOOT_DEVICE"]="253:0"
__sb_vars["BASIC_KERNEL_CMDLINE"]="console=ttyS0,115200n1 rootfstype=squashfs loglevel=8"
__sb_vars["KERNEL_INITRAMFS"]=""
__sb_vars["KERNEL"]="append-opteenode \$\$(KDIR)/image-\$\$(firstword \$\$(DEVICE_DTS)).dtb | \
kernel-bin | lzma | squashfs-hashed | fw-ar-ver | hkdf | \
fit-sign lzma \$\$(KDIR)/image-sb-\$\$(firstword \$\$(DEVICE_DTS)).dtb"

__enable_kernel_config() {
	local config_path="$1"; shift
	local options="$1"; shift
	local value="$1"; shift
	local option=

	[ -z "$value" ] && value="y"

	for option in ${options};
	do
		echo "${option}=${value}" >> ${config_path}
	done
}

__enable_sdk_config() {
	local config_path="$1"; shift
	local options="$1"; shift
	local value="$1"; shift
	local option=
	local sed_cmd=

	[ -z "$value" ] && value="y"

	for option in ${options};
	do
		sed_cmd="sed -i '/^${option}=/d' ${config_path}"
		eval $sed_cmd
		sed_cmd="sed -i '/# ${option} /d' ${config_path}"
		eval $sed_cmd
		echo "${option}=${value}" >> ${config_path}
	done
}

prepare_secure_boot() {
	local arg=
	local sb_path=${BUILD_DIR}/autobuild/filogic/security/secure_boot

	echo "Preparing Secure Boot..."

	for arg in $*; do
		case "${arg%%=*}" in
		"fit_key_dir" | "fit_key_name")
			if [ -n "${arg#*=}" ]; then
				__sb_vars[$(echo ${arg%%=*} | tr '[:lower:]' '[:upper:]')]="${arg#*=}"
			fi
			;;
		*)
			;;
		esac
	done

	# copy common sb files
	cp -fpR ${sb_path}/common/target/ ${BUILD_DIR}
	cp -fpR ${sb_path}/common/package/ ${BUILD_DIR}
}

prepare_optee() {
	local arg=
	local plat=
	local optee_path=${BUILD_DIR}/autobuild/filogic/security/optee

	echo "Preparing OP-TEE..."

	for arg in $*; do
		case "${arg%%=*}" in
		"plat")
			plat="${arg#*=}"
			;;
		*)
			;;
		esac
	done

	case ${plat} in
	"mt7988")
		cp -fpR ${optee_path}/plat/${plat}/target ${BUILD_DIR}
		;;
	*)
		echo "${plat} does not support OP-TEE"
		return;
		;;
	esac
}

prepare_fw_encryption() {
	local arg=

	for arg in $*; do
		case "${arg%%=*}" in
		"roe_key_dir" | "roe_key_name")
			if [ -n "${arg#*=}" ]; then
				__sb_vars[$(echo ${arg%%=*} | tr '[:lower:]' '[:upper:]')]="${arg#*=}"
			fi
			;;
		*)
			;;
		esac
	done
}

prepare_security() {
	local plat=$(sed -e 's/\([^-]*\)-\(.*\)/\1/g' <<< "$1")

	case "${plat}" in
	"mt7981" | "mt7986" | "mt7988")
		prepare_secure_boot fit_key_dir=${fit_key_dir} fit_key_name=${fit_key_name}
		prepare_optee plat=${plat}
		;;
	*)
		;;
	esac
}

enable_secure_boot() {
	local arg=
	local sdk_config=${BUILD_DIR}/.config
	local targets=() target_packages=()
	local target= target_package= packages=

	echo "Enabling Secure Boot..."

	# select sb targets according to targets selected by .config
	while read -r target; do
		targets+=("${target}")
	done <<< $(sed -n 's/\(CONFIG_TARGET_DEVICE_.*\)=y/\1/p' "${sdk_config}")

	for target in "${targets[@]}"; do
		__enable_sdk_config ${sdk_config} "${target}-sb" y
	done

	while read -r target_package; do
		target_packages+=("${target_package}")
	done <<< $(sed -n 's/\(CONFIG_TARGET_DEVICE_PACKAGES_.*\)=\(.*\)/\1/p' "${sdk_config}")

	for target_package in "${target_packages[@]}"; do
		packages=$(sed -n "s/${target_package}=\(.*\)/\1/p" "${sdk_config}")
		__enable_sdk_config ${sdk_config} "${target_package}-sb" "${packages}"
	done

	__enable_sdk_config ${sdk_config} CONFIG_PACKAGE_dmsetup y
}

enable_optee() {
	local arg=
	local plat=
	local ta_sign_key=
	local sdk_config=${BUILD_DIR}/.config
	local kernel_config=

	echo "Enabling OP-TEE..."

	for arg in $*; do
		case "${arg%%=*}" in
		"plat")
			plat=$(sed -e 's/\([^-]*\)-\(.*\)/\1/g' <<< "${arg#*=}")
			;;
		"ta_sign_key")
			ta_sign_key="${arg#*=}"
			;;
		*)
			;;
		esac
	done

	kernel_config=${BUILD_DIR}/target/linux/mediatek/${plat}/config-5.4

	__enable_kernel_config ${kernel_config} CONFIG_TEE y
	__enable_kernel_config ${kernel_config} CONFIG_OPTEE y
	__enable_kernel_config ${kernel_config} CONFIG_HW_RANDOM_OPTEE y
	__enable_kernel_config ${kernel_config} CONFIG_OPTEE_SHM_NUM_PRIV_PAGES 1

	__enable_sdk_config ${sdk_config} CONFIG_PACKAGE_optee-mediatek y

	if [ -z "${ta_sign_key}" ]; then
		echo "Warning: you are using OP-TEE default TA signing key, do not use this key for release build"
	else
		echo "Choose ${ta_sign_key} as OP-TEE TA signing key"
		__enable_sdk_config ${sdk_config} CONFIG_OPTEE_TA_SIGN_KEY "\"${ta_sign_key}\""
	fi
}

enable_fw_encryption() {
	local arg=
	local sdk_config=${BUILD_DIR}/.config

	__enable_sdk_config ${sdk_config} CONFIG_MTK_KERNEL_ENCRYPTION y
}

already_has_sb_target() {
	local target="$1"; shift
	local sb_targets=("$@")

	for sb_target in "${sb_targets[@]}"; do
		if [ "${target}" == "${sb_target}" ]; then
			echo 1; return
		fi
	done

	echo 0
}

add_sb_target_name() {
	local target_body="$1"

	target_body=$(sed -e 's/\(define Device\/[^ ]*\)/\1-sb/g' <<< "${target_body}")
	target_body=$(sed -e 's/\(DEVICE_MODEL\s*[:+]\?=\s*[^ ]*\)\(.*\)$/\1-sb\2/g' <<< "${target_body}")

	echo "${target_body}"
}

specify_rootfs_name() {
	local target_body="$1"
	local rootfs_name="rootfs=\$\$\$\$(IMAGE_ROOTFS)-hashed-\$\$(firstword \$\$(DEVICE_DTS))"

	target_body=$(sed -e "/IMAGE\/factory.bin\s*[:+]\?=\s*append-ubi\s*${rootfs_name}.*$/! \
			     s/\(IMAGE\/factory.bin\s*[:+]\?=\s*append-ubi\s*\)rootfs=[^ ]*\(.*\)/\1${rootfs_name} \2/g; \
			     /IMAGE\/factory.bin\s*[:+]\?=\s*append-ubi\s*${rootfs_name}.*$/! \
			     s/\(IMAGE\/factory.bin\s*[:+]\?=\s*append-ubi\s*\)\(.*\)/\1${rootfs_name} \2/g" <<< "${target_body}")

	target_body=$(sed -e "/IMAGE\/sysupgrade.bin\s*[:+]\?=\s*sysupgrade-tar\s*${rootfs_name}.*$/! \
			     s/\(IMAGE\/sysupgrade.bin\s*[:+]\?=\s*sysupgrade-tar\s*\)rootfs=[^ ]*\(.*\)/\1${rootfs_name} \2/g; \
			     /IMAGE\/sysupgrade.bin\s*[:+]\?=\s*sysupgrade-tar\s*${rootfs_name}.*$/! \
			     s/\(IMAGE\/sysupgrade.bin\s*[:+]\?=\s*sysupgrade-tar\s*\)\(.*\)/\1${rootfs_name} \2/g" <<< "${target_body}")

	echo "${target_body}"
}

append_sb_dev_packages() {
	local target_body="$1"
	local dev_packages=
	local package=
	local sb_dev_packages=("uboot-envtools" "dmsetup")

	dev_packages=$(sed -n '/DEVICE_PACKAGES\s*[:+]\?=\s*.*$/{:a;N;/\\$/ba;s/\n//g;p}' <<< "${target_body}")
	if [ -n "${dev_packages}" ]; then
		for package in "${sb_dev_packages[@]}"; do
			if ! grep -E -q "${package}" <<< "${dev_packages}" ; then
				target_body=$(sed -e "s/\(DEVICE_PACKAGES\s*[:+]\?=\s*\)\(.*\)$/\1${package} \2/g" <<< "${target_body}")
			fi
		done
	else
		target_body=$(sed -e "s/endef/  DEVICE_PACKAGES :=$(printf ' %s' ${sb_dev_packages[@]})\nendef/g" <<< "${target_body}")
	fi

	echo "${target_body}"
}

add_sb_vars() {
	local target_body="$1"
	local target="$2"
	local key=

	for key in "${!__sb_vars[@]}"; do
		if [[ "${key}" == "HASHED_BOOT_DEVICE" && ( "${target}" == *"-emmc"* || "${target}" == *"-sd"* ) ]]; then
			continue;
		fi
		target_body=$(sed -e "s#endef#  ${key} := ${__sb_vars[${key}]}\nendef#g" <<< "${target_body}")
	done

	echo "${target_body}"
}

add_default_dev_vars() {
	local default_dev_vars="DEFAULT_DEVICE_VARS +="
	local sb_default_dev_vars=("FIT_KEY_DIR" "ROE_KEY_DIR" "FIT_KEY_NAME" "ROE_KEY_NAME" \
				   "FIRMWARE_ENC_ALGO" "ANTI_ROLLBACK_TABLE" "AUTO_AR_CONF" \
				   "HASHED_BOOT_DEVICE" "BASIC_KERNEL_CMDLINE")
	local var=

	for var in "${sb_default_dev_vars[@]}"
	do
		default_dev_vars+=" ${var}"
	done

	default_dev_vars+="\n"

	echo ${default_dev_vars}
}

generate_sb_target() {
	local target_body="$1"
	local target="$2"

	target_body=$(add_sb_target_name "${target_body}")

	target_body=$(specify_rootfs_name "${target_body}")

	target_body=$(append_sb_dev_packages "${target_body}")

	target_body=$(add_sb_vars "${target_body}" "${target}")

	echo "${target_body}"
}

generate_sb_targets() {
	local plat=$(sed -e 's/\([^-]*\)-\(.*\)/\1/g' <<< "$1")
	local makefile=${BUILD_DIR}/target/linux/mediatek/image/${plat}.mk
	local contents=
	local has_sb_target=
	local sb_target_contents="\n"
	local targets=() sb_targets=()
	local target= sb_target= target_body=

	# get existed sb targets
	while read -r sb_target; do
		sb_targets+=("${sb_target}")
	done <<< $(sed -n 's/define Device\/\([^ ]*\)-sb\s*$/\1/p' "${makefile}")

	# remove existed sb targets
	contents=$(sed '/define Device\/[^ ]*-sb\s*$/,/endef/{d}; /TARGET_DEVICES\s*+=\s*[^ ]*-sb\s*$/{d}' "${makefile}")

	# get normal targets
	while read -r target; do
		targets+=("${target}")
	done <<< $(sed -n 's/define Device\/\([^ ]*\)\s*$/\1/p' <<< "${contents}")

	for target in "${targets[@]}"; do
		# skip normal targets, which already has sb target
		has_sb_target=$(already_has_sb_target ${target} ${sb_targets[@]})
		if [ $has_sb_target -eq 1 ]; then
			continue;
		fi

		target_body=$(sed -n "/define Device\/${target}\s*$/,/endef/p" <<< "${contents}")
		target_body=$(generate_sb_target "${target_body}" "${target}")

		sb_target_contents+="${target_body}\n"
		sb_target_contents+="TARGET_DEVICES += ${target}-sb\n\n"
	done

	# check if we need to add DEFAULT_DEVICE_VARS for sb
	if ! grep -E -q "DEFAULT_DEVICE_VARS" <<< "${contents}"; then
		sb_target_contents+=$(add_default_dev_vars)
	fi

	echo -e "${sb_target_contents}" >> "${makefile}"
}
