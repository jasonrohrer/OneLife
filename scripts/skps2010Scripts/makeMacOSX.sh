#!/bin/sh

if [ $# -lt 1 ] ; then
   echo "Usage: $0 release_name"
   exit 1
fi

./skps2010Build.sh

python3 -m PyInstaller --onefile translator.py
cp dist/translator .

cd OneLife/build
./makeDistributionMacOSX v$1 _ /usr/local/opt/sdl12-compat/lib/libSDL-1.2.0.dylib

