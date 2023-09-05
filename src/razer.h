#ifndef RAZERCTL_RAZER_H
#define RAZERCTL_RAZER_H

#include <sched.h>
#include <stdint.h>

#ifndef _WIN32
#include <time.h>
#define RAZER_SLEEP                    \
    {                                  \
        struct timespec sleep_time;    \
        sleep_time.tv_sec = 0;         \
        sleep_time.tv_nsec = 31100000; \
        nanosleep(&sleep_time, NULL);  \
    }
#else
#define RAZER_SLEEP Sleep(31)
#endif

#define MAX_DPI_STAGES 5

struct mouse_dpi
{
    uint16_t x;
    uint16_t y;
};

struct razerctl_settings
{
    uint8_t device;
    uint16_t polling_rate;
    uint16_t idle_time;
    uint8_t battery_threshold;
    uint8_t lod; // lift-off distance
    uint8_t ld;  // landing distance
    uint8_t stages_count;
    uint8_t active_stage;
    struct mouse_dpi dpi;
    struct mouse_dpi dpi_stages[MAX_DPI_STAGES];
    uint8_t json : 1;
    uint8_t print_devices : 1;
    uint8_t set_polling_rate : 1;
    uint8_t set_idle_time : 1;
    uint8_t set_battery_threshold : 1;
    uint8_t set_active_stage : 1;
    uint8_t set_lod : 1;
    uint8_t set_async_lod : 1;
    uint8_t set_dpi : 1;
    uint8_t set_dpi_stages : 1;
#if defined(RAZERCTL_USE_LIBUSB) && defined(RAZERCTL_USE_HIDRAW)
    uint8_t libusb : 1;
#endif
};

/**
 * Transaction ID used to group request-response, device useful when multiple
 * devices are on one usb Remaining Packets is the number of remaining packets
 * in the sequence Protocol Type is always 0x00 Data Size is the size of
 * payload, cannot be greater than 80. 90 = header (8B) + data + CRC (1B) +
 * Reserved (1B) Command Class is the type of command being issued Command ID
 * is the type of command being send. Direction 0 is Host->Device, Direction 1
 * is Device->Host. AKA Get LED 0x80, Set LED 0x00
 *
 */
struct razer_report
{
    uint8_t status;
    uint8_t transaction_id;
    uint16_t remaining_packets;
    uint8_t protocol_type;
    uint8_t data_size;
    uint8_t command_class;
    uint8_t command_id;
    uint8_t arguments[80];
    uint8_t crc;
    uint8_t reserved;
};

char *razer_strdup(const char *string);

uint8_t razer_round5(uint8_t value);

uint8_t razer_data_crc(struct razer_report *report);

#if defined(RAZERCTL_USE_LIBUSB) && defined(RAZERCTL_USE_HIDRAW)
#include <stdbool.h>
int razer_send_request(void *device, struct razer_report *request, struct razer_report *response, bool libusb);
#else
int razer_send_request(void *device, struct razer_report *request, struct razer_report *response);
#endif

uint8_t razer_parse_dpi_stages(struct razer_report *response, uint8_t *active_stage, struct mouse_dpi *dpi_stages);

int razer_device_iterate(struct razerctl_settings *settings, size_t id, void *device);

int razer_device_info(struct razerctl_settings *settings, size_t id, void *device);

void razer_print_error(struct razer_report *report, const char *message);

void get_serial(struct razer_report *report);

void get_firmware_version(struct razer_report *report);

void get_polling_rate(struct razer_report *report);
void get_polling_rate2(struct razer_report *report);

void set_polling_rate(struct razer_report *report, uint16_t polling_rate);
void set_polling_rate2(struct razer_report *report, uint16_t polling_rate, uint8_t argument);

void get_battery_level(struct razer_report *report);
void get_charging_status(struct razer_report *report);

void get_dpi_xy(struct razer_report *report);
void set_dpi_xy(struct razer_report *report, uint16_t dpi_x, uint16_t dpi_y);

void get_dpi_stages(struct razer_report *report, uint8_t variable_storage);
void set_dpi_stages(struct razer_report *report, uint8_t quantity, uint8_t active_stage,
                    const struct mouse_dpi *dpi_stages);

void get_idle_time(struct razer_report *report);
void set_idle_time(struct razer_report *report, uint16_t idle_time);

void get_low_battery_threshold(struct razer_report *report);
void set_low_battery_threshold(struct razer_report *report, uint8_t threshold);

void get_lod(struct razer_report *report);
void set_lod(struct razer_report *report, uint8_t lod);
void set_lod_async_off(struct razer_report *report);
void set_lod_async_on(struct razer_report *report);
void set_lod_async_on_step2(struct razer_report *report);
void set_async_lod(struct razer_report *report, uint8_t lod, uint8_t ld);

uint8_t clamp_u8(uint8_t value, uint8_t min, uint8_t max);
uint16_t clamp_u16(uint16_t value, uint16_t min, uint16_t max);

/**
 * Status:
 * 0x00 New Command
 * 0x01 Busy
 * 0x02 Successful
 * 0x03 Failure
 * 0x04 No Response / Timeout
 * 0x05 Not Support
 */
enum razer_request_status
{
    RAZER_NEW = 0x00,
    RAZER_BUSY = 0x01,
    RAZER_SUCCESSFUL = 0x02,
    RAZER_FAILURE = 0x03,
    RAZER_TIMEOUT = 0x04,
    RAZER_NOT_SUPPORTED = 0x05
};

enum razer_mouse_lod
{
    RAZER_LOD_LOW = 0x00,
    RAZER_LOD_MEDIUM = 0x01,
    RAZER_LOD_HIGH = 0x02
};

#endif
