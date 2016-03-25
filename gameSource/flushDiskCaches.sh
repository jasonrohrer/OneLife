sync
echo 3 > /proc/sys/vm/drop_caches
blockdev --flushbufs /dev/sda
hdparm -f /dev/sda
# echo 1 > /sys/block/sda/device/delete