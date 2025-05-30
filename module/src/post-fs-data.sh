#!/system/bin/sh

MODDIR=${0%/*}
cd "$MODDIR" || exit

chmod +x $MODDIR/bin -R


#  must usr &
./bin/zygiskd unix_socket d63138f231 &
#./bin/adi -m -c   $MODDIR/zygisk3.json &
./bin/adi  -m  -p 1 --exec /system/bin/app_process64  --injectSoPath /data/adb/modules/zygiskADI/lib/arm64-v8a/libzygisk.so --injectFunSym entry --injectFunArg d63138f231 --monitorCount 10&

