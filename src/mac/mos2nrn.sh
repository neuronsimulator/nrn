#!/usr/bin/env bash
# Mac os x intermediate between applescript and mos2nrn
echo -e "\033]0;NEURON launched from browser\007"

if test "$1" = "" ; then
	echo "The purpose of mos2nrn is to unzip, build, and launch
	models downloaded from the ModelDB model database at
	https://modeldb.science"
	echo "Drop a nrnzip (or zip) file containing a NEURON model onto mos2nrn."
	echo "Press the return key to exit."
	read a
	exit 0
fi

MOS2NRN="mos2nrn2.sh"

"${MOS2NRN}" "$1" "/tmp/mos2nrn".$$ "0"
