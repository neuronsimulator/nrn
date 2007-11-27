#!/bin/sh

#NETSCAPE='/u3/local/hines/netscape/netscape'
NETSCAPE=`which netscape`
help='http://www.neuron.yale.edu/neuron/help'

#help="file:$NEURONHOME/html/help"

#dict=$NEURONHOME/lib/helpdict

#echo "$*"

#url=`sed -n  '/^'"$*"'/{
#	s/.* //p
#	q
#}' $dict`

url=$1

if [ -z "$url" ] ; then
	echo "|$*|"
	url='contents.html'
fi
cmd="$NETSCAPE -remote openURL(${help}/${url})"
if [ -x $NETSCAPE ] ; then 
	$cmd
	if [ ! $? ] ; then
$NEURONHOME/bin/ivdialog "$0: $cmd failed" "Continue" "Continue"
		exit 2
	fi
else
	$NEURONHOME/bin/ivdialog "$0: $NETSCAPE not executable from this shell" "Continue" "Continue"
	exit 1
fi

