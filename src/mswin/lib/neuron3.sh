export N=$1
shift
export NEURONHOME=$N
export PATH=$N/bin:$PATH
source set_nrnpyenv.sh
export PYTHONPATH=$PYTHONPATH:$N/lib/python
if ! $* ; then
echo '
NEURON exited abnormally. Press the return key to close this window
'
read a
fi

