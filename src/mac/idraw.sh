#!/usr/bin/env bash
# Mac os x intermediate between applescript and idraw

echo -e "\033]0;idraw\007"

IDRAW="${NRNHOME}/../iv/x86_64/bin/idraw"
if test "${NRNHOME_EXEC}" != "" ; then
  IDRAW="${NRNHOME_EXEC}/bin/idraw"
fi
cd
"${IDRAW}" "$@"

err=$?
if test "$err" != 0 ; then
  echo "$IDRAW exited with $err"
  echo "Press return key to exit"
  read a
fi
