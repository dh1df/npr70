cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include(pico-sdk/pico_sdk_init.cmake)
project(npr70pi)
pico_sdk_init()

set(WIFI_SSID "${WIFI_SSID}" CACHE INTERNAL "WiFi SSID for examples")
set(WIFI_PASSWORD "${WIFI_PASSWORD}" CACHE INTERNAL "WiFi password for examples")

execute_process(COMMAND
  git describe --always --dirty
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_SHA1
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

set(TUSB_INCLUDE ${PICO_SDK_PATH}/lib/tinyusb/lib/networking)

set(ENC28J60_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pico-enc28j60)
set(ENC28J60_SOURCES
    ${ENC28J60_DIR}/src/enc28j60.c
    ${ENC28J60_DIR}/src/ethernetif.c
)

set(TINYUSB_LIBNETWORKING_SOURCES
    ${PICO_SDK_PATH}/lib/tinyusb/lib/networking/dhserver.c
    ${PICO_SDK_PATH}/lib/tinyusb/lib/networking/rndis_reports.c
)

set(NPR70_COMMON_SOURCES
    ../source/HMI_telnet.cpp
    ../source/DHCP_ARP.cpp
    ../source/global_variables.cpp
    ../source/Eth_IPv4.cpp
    ../source/SI4463.cpp
    ../source/L1L2_radio.cpp
    ../source/TDMA.cpp
    ../source/signaling.cpp
    ../source/main.cpp
)

set(NPR70_PI_SOURCES npr70.cpp tusb_lwip_glue.c usb_descriptors.c W5500_lwip.cpp misc.cpp enchw.cpp littlefs.c wget.c flash.c jsonconf.cpp ping.cpp trace.c)

set_source_files_properties(littlefs-lib/pico-hal.c PROPERTIES COMPILE_DEFINITIONS bool=uint8_t)
add_subdirectory(littlefs-lib)

add_executable(${PROJECT_NAME} ${NPR70_PI_SOURCES} ${NPR70_COMMON_SOURCES} ${ENC28J60_SOURCES} ${TINYUSB_LIBNETWORKING_SOURCES})
target_compile_definitions(${PROJECT_NAME} PRIVATE WIFI_SSID=\"${WIFI_SSID}\" WIFI_PASSWORD=\"${WIFI_PASSWORD}\" FS_SIZE=1048576 -DFW_VERSION="PI DH1DF ${GIT_SHA1}")
target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
#target_link_options(${PROJECT_NAME} PRIVATE -Wl,--wrap=cyw43_cb_tcpip_init)

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
target_include_directories(${PROJECT_NAME} PRIVATE . compat ${ENC28J60_DIR}/include ${ENC28J60_DIR}/lwip ${TUSB_INCLUDE} ${LWIP_INCLUDE} ../source/SI4463)
target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_spi hardware_adc pico_unique_id pico_lwip pico_lwip_http pico_cyw43_arch_lwip_poll tinyusb_device littlefs-lib)
pico_add_extra_outputs(${PROJECT_NAME})
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND cp ${PROJECT_NAME}.bin ${PROJECT_NAME}.sbin COMMAND md5sum ${PROJECT_NAME}.bin >> ${PROJECT_NAME}.sbin)


target_compile_definitions(${PROJECT_NAME} PRIVATE SCENARIO=1)
