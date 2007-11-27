
#support the system("shell command") under mswin
N="`$1/bin/cygpath -u $1`"
export N
shift
PATH=$N/bin:/usr/bin:$PATH
export PATH
fout="`cygpath -u $1`"
shift
eval "$*" > $fout
echo 1 > tmpdos1.tmp



