#ifndef RAZERCTL_DEVICE_H
#define RAZERCTL_DEVICE_H

#include "razer.h"

#define RAZER_VENDOR_ID 0x1532
#define RAZER_USB_REPORT_LEN 0x5A
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
    uint8_t pid;                   // USB Product ID
    struct device_attributes attr; // Device Attributes
};

static const struct razer_device razer_mouse[] = {
    { "Razer Viper V2 Pro (Wired)", 0x00A5, { .battery = 1, .dpi_stages = 1, .hyper_polling = 0, .async_lod = 1 } },
    { "Razer Viper V2 Pro (Wireless)", 0x00A6, { .battery = 1, .dpi_stages = 1, .hyper_polling = 0, .async_lod = 1 } },
    { "Razer DeathAdder V3 Pro (Wired)", 0x00B6, { .battery = 1, .dpi_stages = 1, .hyper_polling = 0, .async_lod = 1 } },
    { "Razer DeathAdder V3 Pro (Wireless)",
      0x00B7,
      { .battery = 1, .dpi_stages = 1, .hyper_polling = 0, .async_lod = 1 } },
    { "Razer HyperPolling Wireless Dongle",
      0x00B3,
      { .battery = 1, .dpi_stages = 1, .hyper_polling = 1, .async_lod = 1 } },
    { "Razer Viper 8KHz", 0x0091, { .battery = 0, .dpi_stages = 1, .hyper_polling = 1, .async_lod = 0 } },
    { "Razer Mamba Elite (Wired)", 0x006C, { .battery = 0, .dpi_stages = 1, .hyper_polling = 0, .async_lod = 0 } },
    { "Razer DeathAdder Chroma", 0x0043, { .battery = 0, .dpi_stages = 0, .hyper_polling = 0, .async_lod = 0 } }
};

#endif // RAZERCTL_DEVICE_H
