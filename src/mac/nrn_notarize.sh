#!/usr/bin/env bash
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

# you should get
# 2021-01-31 17:43:58.537 altool[3021:3231297] CFURLRequestSetHTTPCookieStorageAcceptPolicy_block_invoke: no longer implemented and should not be called
# No errors uploading '/Users/michaelhines/neuron/notarize/build/src/mac/build/NEURON.pkg'.
# RequestUUID = dc9dd4dd-a942-475a-ab59-4996f938d606

# and eventually get an email in just a few minutes saying that
# Your Mac software has been notarized.

# At this point the software can be installed. However it is recommended
# xcrun stapler staple "$pkg"
# so that Gatekeeper will be able to find the whitelist in the file itself
# without the need to perform an online check.
# However it is unclear how long the wait will be if you do a wait loop over
# xcrun altool --notarization-info $RequestUUID \
#  --username "michael.hines@yale.edu" \
#  --password "$app_specific_password"
# until
#   Status: in progress
# changes to
#   Status: success
# at which point it is ok to run the stapler.

# For now, echo a suggestion about waiting for an email and what stapler
# command to execute manually.

echo "
After getting an email from Apple stating that:
  Your Mac software has been notarized.
manually execute

 xcrun stapler staple \"$pkg\"

so that Gatekeeper will be able to find the whitelist in the file itself
without the need to perform an online check.
"

# If the notarization fails. E.g. the message from Apple is
#    "Your Mac software was not notarized"
# then review the notarization LogFileURL by obtaining the log url with
# xcrun altool --notarization-info $RequestUUID \
#  --username "michael.hines@yale.edu" \
#  --password "$app_specific_password"
# and address the issues it shows and try again.
