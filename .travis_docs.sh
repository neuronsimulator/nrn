set -e

git submodule update --init

# Py deps
pip install sphinx sphinx_rtd_theme jupyter nbconvert

(cd docs/tutorials && bash build_rst.sh)
make -C docs html
