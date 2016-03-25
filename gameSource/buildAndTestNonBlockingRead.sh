g++ -o testNonBlockingRead testNonBlockingRead.cpp -I../.. ../../minorGems/io/file/linux/PathLinux.o

sudo ./flushDiskCaches.sh

./testNonBlockingRead

time ./testNonBlockingRead 20MegFile