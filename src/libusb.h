#ifndef RAZERCTL_LIBUSB_H
#define RAZERCTL_LIBUSB_H

#include "razer.h"

#include <libusb-1.0/libusb.h>

#define HID_REQ_GET_REPORT 0x01
#define HID_REQ_SET_REPORT 0x09

struct razer_libusb_device
{
    size_t id;
    libusb_device *device;
};

libusb_device_handle *razer_libusb_open(libusb_device *device, size_t id);

int razer_libusb_init(struct razerctl_settings *settings);

int razer_libusb_get_response(void *device, struct razer_report *request_report, struct razer_report *response_report);

#endif // RAZERCTL_LIBUSB_H
