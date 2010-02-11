
N="`$1/bin/cygpath -u $1`"
PATH=$N/bin
export PATH
export N
NEURONHOME=$N
export NEURONHOME
fname="`cygpath -u $2|sed 's/\.[mM][oO][dD]$/.mod/'`"
if modlunit $fname ; then
echo ""
else
echo ""
echo "Press Return key to exit"
read a
fi
#read a
