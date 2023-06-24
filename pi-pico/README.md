Port of NPR70 to the pi pico
Buid instructions:
In this directory:
git submodule update --init
cd pico-sdk
git submodule update --init
cd ..
mkdir build
cd build
cmake ..
make
Flash npr70pi.uf2
telnet 192.168.0.253 (either via usb or ethernet)
type "bootloader" to reboot into bootloader

Improvements against nucleo NPR70:
- PI Pico is cheaper
- Uses LWIP together with ENC28J60 or USB instead of W5500 for networking: Cheaper and much more flexible
- Doesn't need external SRAM
- Improved telnet cli with help
- Config in json, easy to extend, or to download via wget
- Possibility to update firmware from net
- Ability to trace sent/received packets via USB, wireshark plugin included
- Remote Managebility without additional hardware [WIP]
