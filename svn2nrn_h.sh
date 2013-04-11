#!/bin/sh
major=7
minor=4
a=$1
if test "$a" = "" ; then
	a=.
fi
b=0
c=unknown
d=""
last=0

echo "#define NRN_MAJOR_VERSION \"$major\""
echo "#define NRN_MINOR_VERSION \"$minor\""

if svnversion $a > /dev/null && test -d $a/.svn ; then
	b=`svnversion $a`
	c="`svn info $a | sed -n '
	  /^URL:/s/.*\///p
	  ' | sed 's/%20/ /'`"
	d="`svn info $a | sed -n '
	  /^Last Changed Date:/s/[^:]*: \([^ ]*\).*/\1/p
	  '`"
	last="`svn info $a | sed -n '
	  /^Last Changed Rev:/s/[^:]*: \([0-9]*\).*/\1/p
	  '`"
	httpurl=$a
	if svn info $a | grep -q 'URL.*svn+ssh:' ; then
		httpurl="`svn info $a | sed -n '
			/^URL:/{
				s/^URL.*svn+ssh:/http:/
				s/hines@//
				s/\/home\/svnroot\//\/svn\//p
			}
		'`"
	fi
	if test "$c" = "trunk" ; then
		c=""
	fi
	tno=`svn log -v -q --stop-on-copy $httpurl | grep '^r' | wc -l | sed 's/[ ]*//'`
	echo "#define SVN_DATE \"$d\""
	echo "#define SVN_BRANCH \"$c\""
	echo "#define SVN_CHANGESET \"($b)\""
	echo "#define SVN_TREE_CHANGE \"$tno\""
	echo "#define SVN_BASE_CHANGESET \"$last\""
else
	echo "#define SVN_DATE \"1999-12-31\""
	echo "#define SVN_BRANCH \"\""
	echo "#define SVN_CHANGESET \"($b)\""
	echo "#define SVN_TREE_CHANGE \"0\""
	echo "#define SVN_BASE_CHANGESET \"0\""
	exit 1
fi

