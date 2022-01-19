#!/bin/bash
# ------------------------------------------------------------
# Script to generate the html and .rst stubs to show the
# ipynb within Sphinx generated docs
#
# Requires jupyter nbconvert
# ------------------------------------------------------------
set -e

if [ $# -ge 2 ]; then
  EXECUTE=" --execute"
else
  EXECUTE=""
fi

# Create a clean index.rst
title=$1
echo "
$1
=========

.. toctree::
   :maxdepth: 2
" > index.rst

for filename in *.ipynb; do
    jupyter nbconvert --to html ${EXECUTE} "${filename}"
    name="${filename%.*}"

    # Create stub rst
    echo "$name
----------------

.. raw:: html
   :file: $name.html
" > "$name.rst"

    # Add to index
    echo "   $name" >> index.rst

done
