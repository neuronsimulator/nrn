set -e

echo "Submodule update"
git submodule update --init

echo "Build tutorials"
(cd docs/tutorials && pwd && bash build_rst.sh)

