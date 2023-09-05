cmake_minimum_required(VERSION 3.14)

project(razerctl VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)

option(HIDRAW "Build with HIDRAW support" ON)
option(LIBUSB "Build with LibUSB support" ON)

add_executable(razerctl src/main.c src/razer.c)

if(LIBUSB)
    add_definitions(-DRAZERCTL_USE_LIBUSB)
    target_sources(razerctl PUBLIC src/libusb.c)
    target_link_libraries(razerctl usb-1.0)
endif()

if(HIDRAW)
    add_definitions(-DRAZERCTL_USE_HIDRAW)
    target_sources(razerctl PUBLIC src/hidraw.c)
    target_link_libraries(razerctl udev)
endif()
