# A script to loop over the available pythons installed
# in a manylinux image and build wheels
# Note: It should be placed in parent directory
# PREREQUESITES:
#  - cmake (>=3.5)
#  - flex

set -xe

build_wheel() {
  local py_bin="$1"
  local py_ver=$("$py_bin" -c "import sys; print('%d%d' % tuple(sys.version_info)[:2])")
  local venv_dir="venv$py_ver"

  if [ "$py_ver" = "34" ]; then
    echo "python34 no longer supported"
    return 0
  fi

  "$py_bin" -m venv "$venv_dir"
  . "$venv_dir/bin/activate"
  pip install -U setuptools pip wheel==0.34.1
  pip install git+https://github.com/ferdonline/auditwheel@fix/rpath_append

  pushd nrn
  rm -rf dist build
  pip install -i https://nero-mirror.stanford.edu/pypi/web/simple -r build_requirements.txt
  python setup.py bdist_wheel
  auditwheel repair dist/*.whl
  popd
  deactivate
}

yum install -y flex libXcomposite-devel.x86_64
export PATH=/opt/openmpi/bin:$PATH

for py_bin in /opt/python/cp3*/bin/python; do
  build_wheel "$py_bin"
done

