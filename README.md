ev-tools - Kernel devices emulation
===================================

This package was developed on top of evemu tool mentioned below. It provide record and replay multiple devices at same time with new command line:
* ev-record
* ev-replay

Usage
-----
- record with devices:

    ./ev-record -m /dev/input/event14 -d /dev/input/event15 -x 2000 -y 2000 > record.txt
    
    Above command will record mouse and keyboard event, with initial mouse position set to (2000,2000). Please note you can use ev-record -l to list all devices, and then select mouse device with -m option.

- replay with recording file

    ./ev-replay < record.txt
    Above command will replay all your recorded events. It will create uinput device so you can run this on a device/computer even without the actual device.

evemu - Kernel device emulation
-------------------------------

The evemu library and tools are used to describe devices, record
data, create devices and replay data from kernel evdev devices.

* http://wiki.freedesktop.org/wiki/Evemu
* http://cgit.freedesktop.org/evemu/


