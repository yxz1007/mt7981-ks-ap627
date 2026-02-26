#!/bin/bash
# use roe-key to encrypt rootfs-key, and generate fit-secret
usage() {
	printf "Usage: %s -i input key file -o hkdf key -t its " "$(basename "$0")"
	printf "\n\t-i ==> input key file"
	printf "\n\t-o ==> output key file"
	printf "\n\t-t ==> salt file"
	exit 1
}

bin2hex() {
	od -An -t x1 -w128 | sed "s/ //g"
}

hex2bin() {
	$PERL -e 'print pack "H*", <STDIN>'
}

hkdf() {
	local key=$1
	local salt=$2
	local out=$3
	local k_derived=$(${OPENSSL} kdf -keylen 32 -kdfopt digest:SHA2-256 \
		-kdfopt hexkey:$key \
		-kdfopt hexsalt:$salt HKDF | sed "s/://g")
	echo $out
	echo -n $k_derived | hex2bin  > $out
}

store_salt() {
	local out=$1
	local salt=$2
	echo ${salt} > ${out}

}

while getopts "i:o:t:" OPTION
do
	case $OPTION in
	i ) KEY_PATH=$OPTARG;;
	o ) HKDF_PATH=$OPTARG;;
	t ) SALT_PATH=$OPTARG;;
	* ) echo "Invalid option passed to '$0' (options:$*)"
	usage;;
	esac
done

if [ ! -f "${KEY_PATH}" ]; then
	echo "key directory not found"
	usage;
fi

if [ -z "${BIN}" ]; then
	echo "bin directory not found"
	exit 1
fi

OPENSSL=$BIN/openssl-3
PERL=$BIN/perl
ROE_KEY=$(cat ${KEY_PATH} | bin2hex)
SALT=$(${OPENSSL} rand -hex 32)

hkdf ${ROE_KEY} ${SALT} ${HKDF_PATH}
store_salt ${SALT_PATH} ${SALT}
