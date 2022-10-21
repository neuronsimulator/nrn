#!/usr/bin/env bash
# extract commit version info from src/nrnoc/nrnversion.h
# and PACKAGE_VERSION from configure.ac

if test "$NSRC" = "" ; then
	NSRC=$HOME/neuron/nrn
fi

if test -f $NSRC/src/nrnoc/nrnversion.h ; then
	VERHFILE=`cat $NSRC/src/nrnoc/nrnversion.h`
elif test -d $NSRC/.git ; then
	VERHFILE=`sh $NSRC/git2nrnversion_h.sh`
fi

GIT=GIT

global=`echo "$VERHFILE" | sed -n '/GIT_CHANGESET/s/.*"\(.*\)".*/\1/p'`

type=`echo "$VERHFILE" | sed -n /${GIT}'_BRANCH/s/.*\(".*"\).*/\1/p' | sed '
s/"Release.*"/rel/
s/"\(.*\)"/\1/
'`
describe=`echo "$VERHFILE" | sed -n /${GIT}'_DESCRIBE/s/.*"\(.*\)".*/\1/p'`
base=`echo $global | sed 's/\+//'`
commit=$global
date=`echo "$VERHFILE" | sed -n /${GIT}'_DATE/s/.*"\(.*\)".*/\1/p'`
micro=`echo $describe | sed -n 's/[0-9]*\.[0-9]*\.\([0-9]*\).*/\1/p'`
nano=''
if echo $describe | grep -q '\-' ; then
  nano=`echo $describe | sed -n 's/[^-]*\-\([0-9]*\).*/\1/p'`
fi 
microdotnano=$micro
if test "$nano" != "" ; then
  microdotnano=$micro.$nano
fi

case $1 in
	"type") echo $type ;;
	"commit") echo ${type}-${commit} ;;
	"describe") echo $describe ;;
	"base") echo $base ;;
	"global") echo $global ;;
	"date") echo $date ;;
	"micro") echo $micro ;;
	"nano") echo $nano ;;
	"microdotnano") echo $microdotnano ;;
	*) echo $describe ;;
esac

