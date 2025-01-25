#!/bin/bash

cd ./deps/bluez-src/bluez-5.79
make install

rm /run/dbus/pid
dbus-daemon --system
/usr/local/libexec/bluetooth/bluetoothd &
#for valgrind:
ulimit -n 65536

#for glib
export G_MESSAGES_DEBUG=all