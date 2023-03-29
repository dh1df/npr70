Attempt to port NPR70 to the pi pico
Buid instructions:
From here:
submodule update --init
mkdir build 
cmake ..
Flash pico_webclient.uf2
telnet 192.168.7.1
type "x" to reboot into bootloader
