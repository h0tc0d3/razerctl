#include "hidraw.h"
#include "device.h"
#include "razer.h"

#include <errno.h>
#include <fcntl.h>
#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

int razer_parse_hid_vid(const char *path, uint16_t *bus_type, uint16_t *vendor_id, uint16_t *product_id)
{

    if (!path) {
        fprintf(stderr, "Error in razer_parse_hid_vid: parameter path is NULL");
        return 0;
    }

    size_t path_len = strlen(path) + 14 + 1;
    char *hidraw_path = (char *) calloc(sizeof(char), path_len);

    if (!hidraw_path) {
        fprintf(stderr, "Error in razer_parse_hid_vid: calloc returned NULL");
        return 0;
    }

    snprintf(hidraw_path, path_len, "%s/device/uevent", path);

    int fd = open(hidraw_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Open failed (%s): %s", hidraw_path, strerror(errno));
        return 0;
    }

    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        fprintf(stderr, "Can't set FD_CLOEXEC to file: %s\n\n", hidraw_path);
        return 0;
    }

    char buffer[1024];
    ssize_t res = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (res < 0 || res >= 1024) {
        fprintf(stderr, "Read failed (%s): %s", hidraw_path, strerror(errno));
        return 0;
    }

    buffer[res] = '\0';
    free(hidraw_path);

    char *token = strtok(buffer, "\n");
    while (token != NULL) {
        char *key = token;
        char *value = strchr(token, '=');
        if (value) {
            *value = '\0';
            value++;
            if (strcmp(key, "HID_ID") == 0) {
                res = sscanf(value, "%hx:%hx:%hx", bus_type, vendor_id, product_id);
                if (res == 3)
                    return 1;
            }
        }
        token = strtok(NULL, "\n");
    }

    return 0;
}

int razer_hidraw_open(const char *path, size_t id)
{

    int fd = open(path, O_RDWR);
    int flags = fcntl(fd, F_GETFL, 0);

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        fprintf(stderr, "%s\nCan't set O_NONBLOCK flag to device: %s\n\n", razer_mouse[id].name, path);
        return -1;
    }

    if (fd < 0) {
        fprintf(stderr, "%s\nError open HIDRAW device: %s\n\n", razer_mouse[id].name, path);
        return -1;
    }

    return fd;
}

int razer_hidraw_init(struct razerctl_settings *settings)
{
    const char *indent;

    if (settings->device)
        indent = "";
    else
        indent = "    ";

    struct udev *udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Couldn't create udev context");
        return -1;
    }

    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "hidraw");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry *device_list = udev_enumerate_get_list_entry(enumerate);

    // find razer devices
    size_t active_devices = 0;
    size_t razer_devices_count = sizeof(razer_mouse) / sizeof(*razer_mouse);
    struct razer_hidraw_device devices[RAZER_MAX_NUM_DEVICES];
    struct udev_list_entry *device_entry;
    udev_list_entry_foreach(device_entry, device_list)
    {
        uint16_t device_vid = 0;
        uint16_t device_pid = 0;
        uint16_t device_bus_type = 0;
        const char *path = udev_list_entry_get_name(device_entry);
        if (!path || !razer_parse_hid_vid(path, &device_bus_type, &device_vid, &device_pid))
            continue;

        if (device_bus_type == BUS_USB && device_vid == RAZER_VENDOR_ID) {
            for (size_t j = 0; j < razer_devices_count; j++) {
                if (razer_mouse[j].pid == device_pid) {
                    struct udev_device *raw_dev = udev_device_new_from_syspath(udev, path);
                    if (!raw_dev)
                        continue;
                    struct udev_device *interface_dev
                      = udev_device_get_parent_with_subsystem_devtype(raw_dev, "usb", "usb_interface");
                    if (!interface_dev) {
                        udev_device_unref(raw_dev);
                        continue;
                    }
                    const char *bInterfaceNumber = udev_device_get_sysattr_value(interface_dev, "bInterfaceNumber");
                    long interface = (bInterfaceNumber) ? strtol(bInterfaceNumber, NULL, 16) : -1;
                    const char *bInterfaceProtocol = udev_device_get_sysattr_value(interface_dev, "bInterfaceProtocol");
                    long interface_protocol = (bInterfaceProtocol) ? strtol(bInterfaceProtocol, NULL, 16) : -1;
                    if (interface != 0 || interface_protocol != 0x02) { // 0x02 - mouse protocol
                        udev_device_unref(raw_dev);
                        continue;
                    }
                    const char *dev_path = udev_device_get_devnode(raw_dev);
                    if (dev_path) {
                        devices[active_devices].device = razer_strdup(dev_path);
                        devices[active_devices].id = j;
                        active_devices++;
                    }
                    udev_device_unref(raw_dev);
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
    udev_enumerate_unref(enumerate);
    udev_unref(udev);

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
            int fd = razer_hidraw_open(devices[i].device, id);
            if (fd > 0) {
                razer_device_iterate(settings, id, &fd);
                close(fd);
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

    for (size_t i = 0; i < active_devices; i++) {
        if (devices[i].device)
            free(devices[i].device);
    }

    return 0;
}

int razer_hidraw_get_response(void *device, struct razer_report *request_report, struct razer_report *response_report)
{

    int fd = (*(int *) device);
    char buffer[RAZER_USB_REPORT_LEN + 1];
    memset(buffer, 0x00, RAZER_USB_REPORT_LEN + 1);

    memcpy(buffer + 1, request_report, RAZER_USB_REPORT_LEN);
    int res = ioctl(fd, HIDIOCSFEATURE(RAZER_USB_REPORT_LEN + 1), buffer);
    if (res != RAZER_USB_REPORT_LEN + 1) {
        fprintf(stderr, "Transfer data to device failed.\n");
        return -1;
    }

    RAZER_SLEEP;

    memset(buffer, 0x00, RAZER_USB_REPORT_LEN + 1);
    res = ioctl(fd, HIDIOCGFEATURE(RAZER_USB_REPORT_LEN + 1), buffer);
    if (res != RAZER_USB_REPORT_LEN + 1) {
        fprintf(stderr, "Read response form device failed. Bytes: %d\n", res);
        return -1;
    }

    memcpy(response_report, buffer + 1, RAZER_USB_REPORT_LEN);
    return 0;
}
