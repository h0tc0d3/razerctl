#include "libusb.h"
#include "device.h"
#include "razer.h"

#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

libusb_device_handle *razer_libusb_open(libusb_device *device, size_t idx)
{
    int result;
    libusb_device_handle *device_handle = NULL;
    libusb_open((libusb_device *) device, &device_handle);
    if (!device_handle) {
        fprintf(stderr, "Error open USB device: %s\n", razer_mouse[idx].name);
        return NULL;
    }

    result = libusb_kernel_driver_active(device_handle, 0);
    if (result != LIBUSB_SUCCESS) {
        result = libusb_detach_kernel_driver(device_handle, 0);
    }

    if (result != LIBUSB_SUCCESS) {
        libusb_close(device_handle);
        device_handle = NULL;
    } else {
        result = libusb_claim_interface(device_handle, 0);
        if (result != LIBUSB_SUCCESS) {
            fprintf(stderr, "usb_claim_interface error %d\n", result);
            libusb_close(device_handle);
            device_handle = NULL;
        }
    }
    return device_handle;
}

int razer_libusb_init(struct razerctl_settings *settings)
{
    libusb_context *context = NULL;
    libusb_device **list = NULL;
    libusb_device_handle *device = NULL;

    struct libusb_device_descriptor desc = { 0 };

    const char *indent;

    if (settings->device)
        indent = "";
    else
        indent = "    ";

    int result = libusb_init(&context);
    if (result < 0) {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(result));
        return -1;
    }

    libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

    ssize_t count = libusb_get_device_list(context, &list);
    if (count <= 0) {
        return 0;
    }

    size_t active_devices = 0;
    struct razer_libusb_device devices[RAZER_MAX_NUM_DEVICES];

    size_t razer_devices_count = sizeof(razer_mouse) / sizeof(*razer_mouse);
    for (ssize_t i = 0; i < count; i++) {
        result = libusb_get_device_descriptor(list[i], &desc);
        if (!result) {
            for (size_t j = 0; j < razer_devices_count; j++) {
                if (desc.idVendor == RAZER_VENDOR_ID && razer_mouse[j].pid == desc.idProduct) {
                    devices[active_devices].device = list[i];
                    devices[active_devices].id = j;
                    active_devices++;
                    if (active_devices >= RAZER_MAX_NUM_DEVICES) {
                        fprintf(stderr,
                                "Device list reached max value. Please edit RAZER_MAX_NUM_DEVICES in device.h file.\n");
                        goto next_step;
                    }
                }
            }
        }
    }

next_step:

    if (settings->json && !settings->device) {
        printf("[\n");
    }

    for (size_t i = 0; i < active_devices; i++) {

        size_t id = devices[i].id;

        if (settings->print_devices) {
            if (settings->json)
                printf("%s{\n%s    \"id\": %zu,\n%s    \"name\": \"%s\"\n", indent, indent, i + 1, indent,
                       razer_mouse[id].name);
            else
                printf("ID: %zu\nName: %s\n\n", i + 1, razer_mouse[id].name);
        } else {
            if (settings->device && settings->device != i + 1)
                continue;
            if (settings->json)
                printf("%s{\n%s    \"id\": %zu,\n%s    \"name\": \"%s\",\n", indent, indent, i + 1, indent,
                       razer_mouse[id].name);
            else
                printf("ID: %zu\nName: %s\n", i + 1, razer_mouse[id].name);
        }

        if (!settings->print_devices) {
            device = razer_libusb_open(devices[i].device, id);
            if (device) {
                razer_device_iterate(settings, id, device);
                libusb_release_interface((libusb_device_handle *) device, 0);
                libusb_attach_kernel_driver((libusb_device_handle *) device, 0);
                libusb_close((libusb_device_handle *) device);
            }
        }

        if (settings->json) {
            if (active_devices - 1 == i || settings->device)
                printf("%s}\n", indent);
            else
                printf("%s},\n", indent);
        }
    }

    if (settings->json && !settings->device) {
        printf("]\n");
    }

    libusb_free_device_list(list, 1);
    libusb_exit(context);
    return 0;
}

int razer_libusb_get_response(void *device, struct razer_report *request_report, struct razer_report *response_report)
{

    int len;
    uint8_t request_type;
    uint8_t buffer[RAZER_USB_REPORT_LEN] = { 0 };

    memcpy(&buffer, request_report, RAZER_USB_REPORT_LEN);

    request_type = LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT; // 0x21
    len = libusb_control_transfer((libusb_device_handle *) device, request_type, HID_REQ_SET_REPORT, 0x300, 0, buffer,
                                  RAZER_USB_REPORT_LEN, 5000);

    if (len != RAZER_USB_REPORT_LEN) {
        fprintf(stderr, "Transfer data to device failed.\n");
        return -1;
    }

    RAZER_SLEEP;

    request_type = LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN; // 0xA1
    len = libusb_control_transfer((libusb_device_handle *) device, request_type, HID_REQ_GET_REPORT, 0x300, 0, buffer,
                                  RAZER_USB_REPORT_LEN, 5000);

    if (len != RAZER_USB_REPORT_LEN) {
        fprintf(stderr, "Read response form device failed. Bytes: %d\n", len);
        return -1;
    }

    memcpy(response_report, buffer, RAZER_USB_REPORT_LEN);
    return 0;
}
