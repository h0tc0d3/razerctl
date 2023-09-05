#ifndef RAZERCTL_DEVICE_H
#define RAZERCTL_DEVICE_H

#include "razer.h"

#define RAZER_VENDOR_ID 0x1532
#define RAZER_VENDOR_ID_TEXT "1532"
#define RAZER_USB_REPORT_LEN sizeof(struct razer_report)
#define RAZER_MAX_NUM_DEVICES 32

struct device_attributes
{
    uint8_t battery : 1;       // Have battery
    uint8_t dpi_stages : 1;    // Can use DPI stages
    uint8_t hyper_polling : 1; // HyperPolling
    uint8_t async_lod : 1;     // Async lift off distance
};

struct razer_device
{
    const char *name;              // Device Name
    uint16_t pid;                  // USB Product ID
    struct device_attributes attr; // Device Attributes
};

extern const struct razer_device razer_mouse[];
extern const size_t razer_devices_count;

#endif // RAZERCTL_DEVICE_H
