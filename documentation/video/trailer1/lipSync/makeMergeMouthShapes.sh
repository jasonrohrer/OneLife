
ROOT_PATH=../../../../..

g++ -g -o mergeMouthShapes -I $ROOT_PATH mergeMouthShapes.cpp $ROOT_PATH/minorGems/io/file/linux/PathLinux.cpp $ROOT_PATH/minorGems/util/StringBufferOutputStream.cpp $ROOT_PATH/minorGems/util/stringUtils.cpp