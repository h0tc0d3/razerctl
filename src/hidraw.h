#ifndef RAZERCTL_HIDRAW_H
#define RAZERCTL_HIDRAW_H

#include "razer.h"

#define BUS_USB 0x03
#define HIDIOCSFEATURE(len) _IOC(_IOC_WRITE | _IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len) _IOC(_IOC_WRITE | _IOC_READ, 'H', 0x07, len)

struct razer_hidraw_device
{
    size_t id;
    char *device;
};

int razer_parse_hid_vid(const char *path, uint16_t *bus_type, uint16_t *vendor_id, uint16_t *product_id);

int razer_hidraw_open(const char *path, size_t id);

int razer_hidraw_init(struct razerctl_settings *settings);

int razer_hidraw_get_response(void *device, struct razer_report *request_report, struct razer_report *response_report);

#endif // RAZERCTL_HIDRAW_H
