# A script to loop over the available pythons installed
# on OSX and build wheels
# Note: It should be involed from nrn directory
# PREREQUESITES:
#  - cmake (>=3.5)
#  - flex
#  - todo

set -e

if [ ! -f setup.py ]; then
    echo "Error: setup.py not found. Please launch $0 from the root dir"
    exit 1
fi


setup_venv() {
    local py_bin="$1"
    local py_ver=$("$py_bin" -c "import sys; print('%d%d' % tuple(sys.version_info)[:2])")
    local venv_dir="venv$py_ver"

    if [ "$py_ver" -lt 35 ]; then
        echo "[SKIP] Python $py_ver no longer supported"
        return 0
    fi

    echo " - Creating $venv_dir: $py_bin -m venv $venv_dir"
    "$py_bin" -m venv "$venv_dir"
    . "$venv_dir/bin/activate"

    if ! pip install -U pip setuptools wheel; then
        curl https://bootstrap.pypa.io/get-pip.py | python
        pip install -U setuptools wheel
    fi
}


build_wheel_linux() {
    echo "[BUILD WHEEL] Building with interpreter $1"
    setup_venv "$1"

    echo " - Installing build requirements"
    pip install git+https://github.com/ferdonline/auditwheel@fix/rpath_append
    pip install -i https://nero-mirror.stanford.edu/pypi/web/simple -r build_requirements.txt

    echo " - Building..."
    rm -rf dist build
    python setup.py build_ext --cmake-prefix=/opt/ncurses --cmake-defs="NRN_MPI_DYNAMIC=/opt/openmpi/include;/opt/mpich/include;/opt/mpt/include" bdist_wheel

    echo " - Repairing..."
    auditwheel repair dist/*.whl

    deactivate
}


build_wheel_osx() {
    echo "[BUILD WHEEL] Building with interpreter $1"
    setup_venv "$1"

    echo " - Installing build requirements"
    pip install -U delocate -r packaging/python/build_requirements.txt

    echo " - Building..."
    rm -rf dist build
    python setup.py build_ext --cmake-defs="NRN_MPI_DYNAMIC=/usr/local/opt/openmpi/include;/usr/local/opt/mpich/include" bdist_wheel

    echo " - Repairing..."
    delocate-wheel -w wheelhouse -v dist/*.whl  # we started clean, there's a single wheel

    deactivate
}


# MAIN

case "$1" in

  linux)
    for py_bin in /opt/python/cp3*/bin/python; do
        build_wheel "$py_bin"
    done
    ;;

  osx)
    for py_bin in /Library/Frameworks/Python.framework/Versions/3*/bin/python3; do
        build_wheel_osx "$py_bin"
    done
    ;;

  *)
    echo "Usage: $(basename $0) < linux | osx >"
    exit 1
    ;;

esac
