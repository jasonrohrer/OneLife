
ROOT_PATH=../../../../..

g++ -g -o genMouthShapes -I $ROOT_PATH genMouthShapes.cpp $ROOT_PATH/minorGems/io/file/linux/PathLinux.cpp $ROOT_PATH/minorGems/sound/formats/aiff.cpp $ROOT_PATH/minorGems/util/StringBufferOutputStream.cpp $ROOT_PATH/minorGems/util/stringUtils.cpp