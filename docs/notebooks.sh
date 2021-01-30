set -e

echo "Converting jupyter notebooks to html"
if [ $# -ge 1 ]; then
  EXECUTE="execute"
  echo "  NOTE: --execute requested"
else
  EXECUTE=""
  echo "  NOTE: notebooks will not be executed"
fi

(cd tutorials && pwd && bash ../build_rst.sh "Python tutorials" ${EXECUTE})
(cd rxd-tutorials && pwd && bash ../build_rst.sh "Python RXD tutorials" ${EXECUTE})

