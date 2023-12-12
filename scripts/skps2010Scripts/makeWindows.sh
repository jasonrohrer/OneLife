#!/bin/sh

if [ $# -lt 1 ] ; then
   echo "Usage: $0 release_name"
   exit 1
fi

# SDL lib for linux-windows cross compile
if [ ! -e SDL-1.2.15 ]
then
    wget https://www.libsdl.org/release/SDL-devel-1.2.15-mingw32.tar.gz
    tar -xvzf SDL-devel-1.2.15-mingw32.tar.gz
    rm SDL-devel-1.2.15-mingw32.tar.gz
fi

if [ ! -e SDL ]
then
    ln -s SDL-1.2.15/include/SDL .
fi

# freetype lib for linux-windows cross compile
if [ ! -e mingw32 ]
then
    wget https://mirror.msys2.org/mingw/mingw32/mingw-w64-i686-freetype-2.13.2-1-any.pkg.tar.zst
    tar --use-compress-program=unzstd -xvf mingw-w64-i686-freetype-2.13.2-1-any.pkg.tar.zst
    rm mingw-w64-i686-freetype-2.13.2-1-any.pkg.tar.zst
fi

if [ ! -e freetype2 ]
then
    ln -s mingw32/include/freetype2 .
fi

if [ ! -e Winsock.h ]
then
    ln -s /usr/i686-w64-mingw32/include/winsock.h Winsock.h
fi

cd OneLife

./configure 5
cd gameSource
make

cd ../build

./makeDistributionWindows v$1

cp ../scripts/skps2010Scripts/translator.py "windows/OneLife_v$1"
cp ../scripts/skps2010Scripts/translator.exe "windows/OneLife_v$1"

mv "windows/OneLife_v$1" "../../"

cd ../../

echo "done building OneLife_v$1"

zip -r -q OneLife_Windows_v$1.zip OneLife_v$1

echo "done zipping OneLife_Windows_v$1.zip"