#!/usr/bin/env bash

# Signing the package with an identified developer certificate means the
# installer does not have to lower the security settings on their machine.
# The information in this file is not private since the installer may obtain
# with
# pkgutil --check-signature /path/to/package.pkg

# "$1" The package to be signed e.g. src/mac/build/NEURON-7.4.pkg

rm -f $1.signed

productsign --sign "Developer ID Installer: Michael Hines (8APKV34642)" \
  $1 $1.signed

echo "mv $1.signed $1"
mv $1.signed $1
