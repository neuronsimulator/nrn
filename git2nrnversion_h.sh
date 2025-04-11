#!/usr/bin/env bash

a=$1
if test "$a" = "" ; then
	a=.
fi

cd $a

# Do the fast directory check before the slow git command
if [ -d .git ] || git rev-parse --git-dir > /dev/null 2>&1; then
        # Official Versioning shall rely on annotated tags (don't use `--tags` or `--all`)
        # (please refer to NEURON SCM documentation)
        describe="`git describe`"
        branch="`git rev-parse --abbrev-ref HEAD`" # branch name
        modified="`git status -s -uno --porcelain | sed -n '1s/.*/+/p'`" # + if modified
        gcs=`git -c log.showSignature=false log --format="%h" -n 1` #short commit hash
        d="`git -c log.showSignature=false log --format="%cd" -n 1 --date=short`" # date
        echo "#define GIT_DATE \"$d\""
        echo "#define GIT_BRANCH \"$branch\""
        echo "#define GIT_CHANGESET \"${gcs}${modified}\""
        echo "#define GIT_DESCRIBE \"${describe}${modified}\""
elif test -f _nrnversion_h ; then
        cat _nrnversion_h
elif test -f src/nrnoc/nrnversion.h ; then
        sed -n '1,$p' src/nrnoc/nrnversion.h
else
        echo "#define GIT_DATE \"Build Time: $(date "+%Y-%m-%d-%H:%M:%S")\""
        echo "#define GIT_BRANCH \"unknown branch\""
        echo "#define GIT_CHANGESET \"unknown commit id\""
        echo "#define GIT_DESCRIBE \"${PROJECT_VERSION}.dev0\""
fi

