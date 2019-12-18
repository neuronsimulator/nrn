[ -f clang-format-diff.py ] || wget https://raw.githubusercontent.com/llvm-mirror/clang/master/tools/clang-format/clang-format-diff.py

git diff -U0 --no-color HEAD^ | clang-format-diff.py -p1 -i

git add -u .
git diff --cached --exit-code

if [ "$?" -ne 0 ]; then
    echo "Needs reformat! Consider installing the precommit-hook with"
    echo "  git config --local core.hooksPath .githooks"
    # In the future this may be run on merge and commit a change in case
fi
