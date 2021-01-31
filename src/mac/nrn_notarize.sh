#!/bin/bash
set -e
# App specific password used to request notarization
password_path="$HOME/.ssh/notarization-password"
if test -f "$password_path" ; then
  app_specific_password=`cat "$password_path"`
else
  echo "\"$password_path\" does not exist"
  exit 1
fi

pkg="$1"
pkgname="$2"

echo "Notarize request"
xcrun altool --notarize-app \
  --primary-bundle-id "edu.yale.neuron.pkg.$pkgname" \
  --username "michael.hines@yale.edu" \
  --password "$app_specific_password" \
  --file "$pkg"

