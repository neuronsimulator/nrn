#!/usr/bin/env bash

auditprogpath=`dirname $0`
pid=$1
auditdir=$2
tmpdir=$auditdir/$pid
tardir=$auditdir/TAR

if [ ! -d $tmpdir ] ; then
	echo "$0: tmpdir:$tmpdir does not exist"
	exit 1
fi

datefile=$tmpdir/datefile
date > $tmpdir/datefile
commandfile=$tmpdir/commands
xopenlist=$tmpdir/xopens
diffile=$tmpdir/diffile
savefile=$tmpdir/savefile
auditlist=$auditdir/auditlist

export datefile commandfile xopenlist savefile

n=0
while read typ str
do
	echo "$str" >> temp
	case $typ in
	0)
		# just a command to save
		echo "$str" >> $commandfile
		;;
	1)
		# manage an xopen
		rcsdiff -e $str > $diffile 2> /dev/null
		case $? in
		0)
			# identical with most recent rcs version
			rev=`rlog -h $str | sed -n '/^head:/s/head://p'`
			echo "0 $str x $rev" >> $xopenlist
			;;
		1)	
			# differs from rcs version so save the diff
			rev=`rlog -h $str | sed -n '/^head:/s/head://p'`
			echo "1 $str f$n $rev" >> $xopenlist
			mv $diffile $tmpdir/f$n
			n=`expr $n + 1`
			;;
		2)
			# no rcs version so save the file
			echo "2 $str f$n x" >> $xopenlist
			cp $str $tmpdir/f$n
			n=`expr $n + 1`
			;;
		esac
		;;
	2)
		# manage execution of an emacs buffer
		echo "3 $str" >> $xopenlist
		;;
	3)
		# save audit
		echo "$str `date`" >> $savefile
		echo "-1 $str x x" >> $xopenlist
		$auditprogpath/saveaudit
		;;
	esac
done


if [ -f $savefile ] ; then
	# save everything
	tarfile=`$auditprogpath/gettarname $tardir`
	cat $tarfile $savefile >> $auditlist
	(cd $tmpdir ; tar cf ../$pid.tar *)
	mv $tmpdir/../$pid.tar $tarfile
	chmod 444 $tarfile
	compress $tarfile
fi

if true ; then
	# clean up
	echo "Removing $tmpdir";
	rm -r $tmpdir
fi
