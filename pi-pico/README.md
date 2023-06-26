Port of NPR70 to the pi pico
Buid instructions:
In this directory:
- git submodule update --init
- cd pico-sdk
- git submodule update --init
- cd ..
- mkdir build
- cd build
- cmake ..
- make
- Flash npr70pi.uf2
- telnet 192.168.0.253 (either via usb or ethernet)
- type "bootloader" to reboot into bootloader

Improvements against nucleo NPR70:
- PI Pico is cheaper
- Uses LWIP together with ENC28J60 or USB instead of W5500 for networking: Cheaper and much more flexible
- Doesn't need external SRAM
- Improved telnet cli with help
- Config in json, easy to extend, or to download via wget
- Possibility to update firmware from net
- Ability to trace sent/received packets via USB, wireshark plugin included
- Remote Managebility without additional hardware [WIP]

Additional commands:
- help: Display list of available commands
- wget: Gets a file into littlefs from network
- ls, rm, cat, cp, sum, mv: Work on littlefs, similar to their unix counterparts
- flash: Flashes a new firmware (.sbin) from littlefs or http-URL
- test: For development purposes :-)
- xset wifi\_id: To set the wifi client id
- xset wifi\_passphrase: To set the wifi client passphrase
- xset client\_int_\size: The number of modem-internal IPs to use
- xdisplay: Display options set by xset
- bootloader: Enter bootloader to flash new firmware via usb
- ping: Similar to unix ping
- trace: Enable RF-packet-tracing to usb network device
- uptime: Show time since last crash ;-)
- free: Show free memory
- display help: Show display options
- display net: Show network configuration
- set help: Show set options
