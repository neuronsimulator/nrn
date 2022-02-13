#!/usr/bin/env bash

# to be executed by retrieve_audit
auditprogpath=`dirname $0`
pid=ret
id=$1
auditdir=$2
tmpdir=$auditdir/$pid
tardir=$auditdir/TAR

if [ -d $tmpdir ] ; then
	exit 1;
fi
mkdir $tmpdir
zcat $tardir/$id.tar.Z | ( cd $tmpdir ; tar xf - )

cd $tmpdir

n=0
echo $tmpdir 
read cmdfile str < savefile
cp $cmdfile $cmdfile.tmp
echo $tmpdir/$cmdfile.tmp

cat xopens  | while true ; do
	read type fname localname rev
#	echo $type $fname $localname $rev 1>&2
	case $type in
	0)
		co -p$rev $fname > $n.tmp 2>/dev/null
		echo $fname
		;;
	1)
		co -p$rev $fname > $n.tmp 2>/dev/null
		(cat $localname ; echo 'w' ; echo 'q') | ed - $n.tmp
		echo $fname
		;;
	2)
		cp $localname $n.tmp
		echo $fname
		;;
	3)
		cp $fname $n.tmp
		echo "em"
		;;
	-1)
		exit 0;
	esac
	echo $tmpdir/$n.tmp
	n=`expr $n + 1`
done
