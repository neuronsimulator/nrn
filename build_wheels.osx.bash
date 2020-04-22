# A script to loop over the available pythons installed
# on OSX and build wheels
# Note: It should be involed from nrn directory
# PREREQUESITES:
#  - cmake (>=3.5)
#  - flex
#  - todo

set -xe

pip3 install -U delocate

for py_bin in /Library/Frameworks/Python.framework/Versions/3.7*/bin/python3; do
    echo "Building wheel with $py_bin"
    $py_bin setup.py build_ext --mpi-dynamic="/usr/local/opt/openmpi/bin;/usr/local/opt/mpich/bin" bdist_wheel
done

for pywheel in `ls dist/*`; do
    echo "fixing $pywheel"
    delocate-wheel -w wheelhouse -v $pywheel
done
