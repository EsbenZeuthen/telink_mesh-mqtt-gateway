#!/bin/bash
rm /run/dbus/pid
dbus-daemon --system
/usr/local/libexec/bluetooth/bluetoothd &
./meshgateway