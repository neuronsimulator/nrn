
#support the system("shell command") under mswin
# a copy of nrnsys.sh but without stdout redirection
N="`$1/bin/cygpath -u $1`"
export N
shift
PATH=$N/bin:/usr/bin:$PATH
export PATH
eval "$*"
echo 1 > tmpdos1.tmp



