#!/system/bin/sh

MODDIR=${0%/*}
chmod +x $MODDIR/bin -R
#  must usr &
./bin/zygiskd unix_socket d63138f231 &
#./bin/adi  $MODDIR/zygisk.json &

