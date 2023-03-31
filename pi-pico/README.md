Attempt to port NPR70 to the pi pico
Buid instructions:
In this directory:
git submodule update --init
mkdir build
cd build
cmake ..
Flash npr70.uf2
telnet 192.168.7.1
type "x" to reboot into bootloader
