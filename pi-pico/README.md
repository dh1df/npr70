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
Flash npr70.uf2
telnet 192.168.0.253 (either via usb or ethernet)
type "bootloader" to reboot into bootloader
