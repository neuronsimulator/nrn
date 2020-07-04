set -e

echo "Submodule update"
git submodule update --init

echo "Build tutorials"
(cd tutorials && pwd && bash ../build_rst.sh "Python tutorials")
(cd rxd-tutorials && pwd && bash ../build_rst.sh "Python RXD tutorials")

