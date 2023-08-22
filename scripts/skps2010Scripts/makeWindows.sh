#!/bin/sh

# SDL lib for linux-windows cross compile
if [ ! -e SDL-1.2.15 ]
then
    wget https://www.libsdl.org/release/SDL-devel-1.2.15-mingw32.tar.gz
    tar -xvzf SDL-devel-1.2.15-mingw32.tar.gz
    rm SDL-devel-1.2.15-mingw32.tar.gz
fi

# if [ ! -e SDL ]
# then
#     ln -s SDL-1.2.15/include/SDL .
# fi

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

mv "windows/OneLife_v$1" "../../"

cd ../../

echo "done building OneLife_v$1"