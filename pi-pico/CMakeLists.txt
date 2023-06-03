cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include(pico-sdk/pico_sdk_init.cmake)
project(npr70)
pico_sdk_init()

set(WIFI_SSID "${WIFI_SSID}" CACHE INTERNAL "WiFi SSID for examples")
set(WIFI_PASSWORD "${WIFI_PASSWORD}" CACHE INTERNAL "WiFi password for examples")



set(TUSB_INCLUDE ${PICO_SDK_PATH}/lib/tinyusb/lib/networking)

set(ENC28J60_DIR ${CMAKE_CURRENT_SOURCE_DIR}/enc28j60driver)
set(ENC28J60_SOURCES
    ${ENC28J60_DIR}/enc28j60driver/enc28j60.c
    ${ENC28J60_DIR}/lwip/netif/mchdrv.c
)
set_source_files_properties(${ENC28J60_SOURCES} PROPERTIES COMPILE_DEFINITIONS bool=uint8_t)

set(TINYUSB_LIBNETWORKING_SOURCES
    ${PICO_SDK_PATH}/lib/tinyusb/lib/networking/dhserver.c
    ${PICO_SDK_PATH}/lib/tinyusb/lib/networking/rndis_reports.c
)

set(NPR70_COMMON_SOURCES
    ../source/HMI_telnet.cpp
    ../source/DHCP_ARP.cpp
    ../source/global_variables.cpp
    ../source/Eth_IPv4.cpp
    # ../source/config_flash.cpp
    ../source/SI4463.cpp
    ../source/L1L2_radio.cpp
    ../source/TDMA.cpp
    ../source/signaling.cpp
    ../source/main.cpp
)

set(NPR70_PI_SOURCES npr70.cpp tusb_lwip_glue.c usb_descriptors.c W5500_lwip.cpp misc.cpp enchw.c littlefs.c wget.c flash.c jsonconf.cpp)

set_source_files_properties(littlefs-lib/pico-hal.c PROPERTIES COMPILE_DEFINITIONS bool=uint8_t)
add_subdirectory(littlefs-lib)

add_executable(${PROJECT_NAME} ${NPR70_PI_SOURCES} ${NPR70_COMMON_SOURCES} ${ENC28J60_SOURCES} ${TINYUSB_LIBNETWORKING_SOURCES})
target_compile_definitions(${PROJECT_NAME} PRIVATE WIFI_SSID=\"${WIFI_SSID}\" WIFI_PASSWORD=\"${WIFI_PASSWORD}\" FS_SIZE=1048576)

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
target_include_directories(${PROJECT_NAME} PRIVATE . compat ${ENC28J60_DIR}/enc28j60driver ${ENC28J60_DIR}/lwip ${TUSB_INCLUDE} ${LWIP_INCLUDE} ../source/SI4463)
target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_spi hardware_adc pico_unique_id pico_lwip pico_lwip_http pico_cyw43_arch_lwip_poll tinyusb_device littlefs-lib)
pico_add_extra_outputs(${PROJECT_NAME})


target_compile_definitions(${PROJECT_NAME} PRIVATE SCENARIO=1)
