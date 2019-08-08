#!/bin/sh

set -e
cd "$(dirname "${0}")"
for REPO in OneLife OneLifeData7; do
  [[ -d "${REPO}" ]] || git clone "https://github.com/twohoursonelife/${REPO}"
  git -C "${REPO}" clean -f -x -d
  git -C "${REPO}" checkout -- .
  git -C "${REPO}" pull
done
for REPO in minorGems; do
  [[ -d "${REPO}" ]] || git clone "https://github.com/jasonrohrer/${REPO}"
  git -C "${REPO}" clean -f -x -d
  git -C "${REPO}" checkout -- .
  git -C "${REPO}" pull
done
cd OneLife
./configure 2
cd gameSource
export CUSTOM_MACOSX_LINK_FLAGS="-F /Library/Frameworks"
make
cd ../build
mkdir mac
./makeDistributionMacOSX "v${1-DEV}" IntelMacOSX /Library/Frameworks/SDL.framework
scp -P 26 mac/2HOL_v${1-DEV}_IntelMacOSX.tar.gz colin@twohoursonelife.com:/var/www/html/downloads/2HOL_v${1-DEV}_IntelMacOSX.tar.gz