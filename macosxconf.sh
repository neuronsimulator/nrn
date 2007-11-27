nver=`./nrnversion.sh`
echo $nver
exit 0
installdir="/Applications/NEURON-${nver}"
IVHOME="${installdir}/iv"
NRNHOME="${installdir}/nrn"
export IVHOME
export NRNHOME
echo ${IVHOME}
echo ${NRNHOME}
../nrn/configure --prefix=${NRNHOME} --with-iv=${IVHOME} --srcdir=../nrn

