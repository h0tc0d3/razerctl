#include "device.h"

const struct razer_device razer_mouse[] = {
    { "Razer Viper V2 Pro (Wired)",
      0x00A5,
      { .battery = 1, .dpi_stages = 1, .hyper_polling = 0, .async_lod = 1 } },
    { "Razer Viper V2 Pro (Wireless)",
      0x00A6,
      { .battery = 1, .dpi_stages = 1, .hyper_polling = 0, .async_lod = 1 } },
    { "Razer DeathAdder V3 Pro (Wired)",
      0x00B6,
      { .battery = 1, .dpi_stages = 1, .hyper_polling = 0, .async_lod = 1 } },
    { "Razer DeathAdder V3 Pro (Wireless)",
      0x00B7,
      { .battery = 1, .dpi_stages = 1, .hyper_polling = 0, .async_lod = 1 } },
    { "Razer HyperPolling Wireless Dongle",
      0x00B3,
      { .battery = 1, .dpi_stages = 1, .hyper_polling = 1, .async_lod = 1 } },
    { "Razer Viper 8KHz",
      0x0091,
      { .battery = 0, .dpi_stages = 1, .hyper_polling = 1, .async_lod = 0 } },
    { "Razer Mamba Elite (Wired)",
      0x006C,
      { .battery = 0, .dpi_stages = 1, .hyper_polling = 0, .async_lod = 0 } },
    { "Razer DeathAdder Chroma",
      0x0043,
      { .battery = 0, .dpi_stages = 0, .hyper_polling = 0, .async_lod = 0 } }
};

const size_t razer_devices_count = sizeof(razer_mouse) / sizeof((razer_mouse)[0]);
