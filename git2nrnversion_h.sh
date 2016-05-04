#!/bin/sh
major=7
minor=5
a=$1
if test "$a" = "" ; then
	a=.
fi

echo "#define NRN_MAJOR_VERSION \"$major\""
echo "#define NRN_MINOR_VERSION \"$minor\""

cd $a

if git log > /dev/null && test -d .git ; then
        lcs="NOT_YET_SUPPORTED" #lcs=`git diff --name-status` # Gives latest status ... not sure that's what we want ...
        branch="`git rev-parse --abbrev-ref HEAD`" # OK
        tags="NOT_YET_SUPPORTED" #"`git tag --contains`" # not sure this will work fine ...
        gcs=`git log --format="%H" -n 1 --date=short` #commit hash
        treehash="`git show -s --pretty=format:%T .`" # get sha1
        d="`git log --format="%ad" -n 1 --date=short`"
        echo "#define GIT_DATE \"$d\""
        echo "#define GIT_BRANCH \"$branch\""
        echo "#define GIT_CHANGESET \"$gcs\""
        echo "#define GIT_TREESET \"$treehash\""
        echo "#define GIT_LOCAL \"$lcs\""
        echo "#define GIT_TAG \"$tags\""
elif test -f src/nrnoc/nrnversion.h ; then
        sed -n '3,$p' src/nrnoc/nrnversion.h
else
        echo "#define GIT_DATE \"1999-12-31\""
        echo "#define GIT_BRANCH \"\""
        echo "#define GIT_CHANGESET \"?\""
        echo "#define GIT_LOCAL \"?\""
        echo "#define GIT_TAG \"\""
        exit 1
fi

