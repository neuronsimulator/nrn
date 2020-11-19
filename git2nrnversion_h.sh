#!/bin/sh

a=$1
if test "$a" = "" ; then
	a=.
fi

cd $a

if git log &> /dev/null && test -d .git ; then
        describe="`git describe --tags`"
        branch="`git rev-parse --abbrev-ref HEAD`" # branch name
        modified="`git status -s -uno --porcelain | sed -n '1s/.*/+/p'`" # + if modified
        gcs=`git log --format="%h" -n 1` #short commit hash
        d="`git log --format="%ad" -n 1 --date=short`" # date
        echo "#define GIT_DATE \"$d\""
        echo "#define GIT_BRANCH \"$branch\""
        echo "#define GIT_CHANGESET \"${gcs}${modified}\""
        echo "#define GIT_DESCRIBE \"${describe}${modified}\""
elif test -f src/nrnoc/nrnversion.h ; then
        sed -n '1,$p' src/nrnoc/nrnversion.h
else
        echo "#define GIT_DATE \"1970-01-01\""
        echo "#define GIT_BRANCH \"unknown branch\""
        echo "#define GIT_CHANGESET \"unknown commit id \""
        echo "#define GIT_DESCRIBE \"no git\""
fi

