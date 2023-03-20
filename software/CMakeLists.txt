cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(GBPUNK C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(GBPUNK)

target_sources(GBPUNK PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/disk/usb_descriptors.c
        ${CMAKE_CURRENT_LIST_DIR}/disk/msc_disk.c
        ${CMAKE_CURRENT_LIST_DIR}/disk/gb_disk.c
        ${CMAKE_CURRENT_LIST_DIR}/gb.c
        ${CMAKE_CURRENT_LIST_DIR}/utils.c
        ${CMAKE_CURRENT_LIST_DIR}/mappers/mbc1.c
        ${CMAKE_CURRENT_LIST_DIR}/mappers/mbc2.c
        ${CMAKE_CURRENT_LIST_DIR}/mappers/mbc3.c
        ${CMAKE_CURRENT_LIST_DIR}/mappers/mbc5.c
        ${CMAKE_CURRENT_LIST_DIR}/mappers/no_mapper.c
        ${CMAKE_CURRENT_LIST_DIR}/mappers/gbcam.c
        ${CMAKE_CURRENT_LIST_DIR}/cart.c
        ${CMAKE_CURRENT_LIST_DIR}/unit_tests.c
        ${CMAKE_CURRENT_LIST_DIR}/status_led.c
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(GBPUNK PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(GBPUNK PUBLIC pico_stdlib hardware_pio tinyusb_device tinyusb_board hardware_flash)

pico_add_extra_outputs(GBPUNK)

# pico_enable_stdio_usb(GBPUNK 1)
# pico_enable_stdio_uart(GBPUNK 1)
