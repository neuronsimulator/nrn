#!/bin/sh
# Mac os x intermediate between applescript and idraw

echo -e "\033]0;idraw\007"

IDRAW="${NRNHOME}/../iv/x86_64/bin/idraw"

cd
"${IDRAW}" "$@"
