#!/bin/sh

sed -n '
	1i\
static JNINativeMethod '$2'_methods[] = {
/Method/s/.*Method: *\(.*\)$/{"\1",/p
/Signature/s/.*Signature: *\(.*\)$/"\1",/p
/JNICALL/s/.*JNICALL *\(.*\)$/(void*)\1},/p
$a\
{0,0,0}\
};
' < $1 | tr -d '\015' > $2.h
