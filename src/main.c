#include "razer.h"
#include "device.h"

#ifdef RAZERCTL_USE_LIBUSB
#include "libusb.h"
#endif
#ifdef RAZERCTL_USE_HIDRAW
#include "hidraw.h"
#endif

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_ARGS(msg)                                            \
    i++;                                                           \
    if (i >= argc || !argv[i] || strncmp(argv[i], "--", 2) == 0) { \
        fprintf(stderr, "%s", msg);                                \
        return -1;                                                 \
    }

/*
 * Minimum and maximum DPI accepted on the command line.
 * Matches the clamp applied by set_dpi_xy() in razer.c, so invalid values
 * are rejected early instead of being silently clamped.
 */
#define DPI_MIN 100
#define DPI_MAX 30000

/**
 * Parse an unsigned integer strictly: reject NULL, empty strings, trailing
 * garbage and out-of-range (ERANGE) values. Returns 0 on success.
 */
static int parse_uint_strict(const char *text, uint64_t *value)
{
    char *end = NULL;

    if (!text)
        return -1;

    errno = 0;
    *value = (uint64_t) strtoull(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0')
        return -1;

    return 0;
}

/**
 * Parse one DPI value of the form "N" or "NxN" into x and y.
 * The backing storage of value is modified in place, so pass a copy.
 * Returns 0 on success; on failure x and y are left untouched.
 */
static int parse_dpi_value(char *value, uint16_t *x, uint16_t *y)
{
    uint64_t parsed_x = 0;
    uint64_t parsed_y = 0;
    char *tail = strchr(value, 'x');

    if (tail) {
        *tail = '\0';
        tail++;
        /* Reject "800x1200x2" style garbage. */
        if (strchr(tail, 'x'))
            return -1;
    }

    if (parse_uint_strict(value, &parsed_x))
        return -1;

    if (tail) {
        if (parse_uint_strict(tail, &parsed_y))
            return -1;
    } else {
        parsed_y = parsed_x;
    }

    if (parsed_x < DPI_MIN || parsed_x > DPI_MAX || parsed_y < DPI_MIN
        || parsed_y > DPI_MAX)
        return -1;

    *x = (uint16_t) parsed_x;
    *y = (uint16_t) parsed_y;
    return 0;
}

/**
 * Check that a polling rate is one of the values any supported device
 * can use. Per-device capability is still checked before talking to the
 * device in razer_device_iterate().
 */
static bool polling_rate_supported(uint16_t polling_rate)
{
    static const uint16_t supported[]
      = { 125, 250, 500, 1000, 2000, 4000, 8000 };

    for (size_t k = 0; k < sizeof(supported) / sizeof(supported[0]); k++) {
        if (supported[k] == polling_rate)
            return true;
    }
    return false;
}

#if defined(RAZERCTL_USE_LIBUSB) && defined(RAZERCTL_USE_HIDRAW)
#define USAGE_LIBUSB "  --libusb\t\tUse libusb instead HIDRAW\n"
#else
#define USAGE_LIBUSB ""
#endif

#define USAGE                                                                                          \
    "Usage: razerctl [OPTION]...\n"                                                                    \
    "  --help\t\tPrint this help and exit\n"                                                           \
    "  --json\t\tOutput format JSON\n" USAGE_LIBUSB                                                    \
    "  --devices\t\tPrint only device list\n"                                                          \
    "  --device\t\tDevice ID (1-32). If not set then all devices\n"                                    \
    "  --lod\t\t\tSet Lift-Off Distance. Available values: low, medium, high.\n"                       \
    "  --async-lod\t\tSet Async Lift-Off Distance. Format: Lift-Off Distance x Landing "               \
    "Distance. 2 <= LOD <= 26 and 1 <= LD <= 25. LOD > LD. Example: 12x10\n"                           \
    "  --polling-rate\tSet polling rate. Available values: 125, 250, 500, 1000, 2000, 4000, 8000\n"    \
    "  --dpi\t\t\tSet DPI (100-30000). Examples: 800 - DPI x=800 y=800, 800x1200 - DPI x=800 y=1200\n" \
    "  --idle-time\t\tSet idle time for power save in seconds. Must be between 60-900\n"               \
    "  --battery-threshold\tSet low battery charge %% threshold for power save. Example: 5 - 100\n"    \
    "  --stage\t\tSet active DPI stage (1-5). Example: 2\n"                                            \
    "  --dpi-stages\t\tSet DPI stages (max 5). Example: 800,1600,3200 or 800x800,1600x1600,3200x3200\n\n"

int main(int argc, char **argv)
{
    int res = 0;
    char *params = NULL;
    struct razerctl_settings settings = { 0 };

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {

            printf(USAGE);
            return 0;

        } else if (!strcmp(argv[i], "--json")) {

            settings.json = 1;

#if defined(RAZERCTL_USE_LIBUSB) && defined(RAZERCTL_USE_HIDRAW)
        } else if (!strcmp(argv[i], "--libusb")) {
            settings.libusb = 1;
#endif
        } else if (!strcmp(argv[i], "--devices")) {

            settings.print_devices = 1;

        } else if (!strcmp(argv[i], "--device")) {

            CHECK_ARGS(
              "Can't set device id. Device ID not passed in parameter.\n\n")

            uint64_t device_id = 0;
            if (parse_uint_strict(argv[i], &device_id)
                || device_id > RAZER_MAX_NUM_DEVICES) {
                fprintf(
                  stderr,
                  "Incorrect device ID: %s. Must be between 0 and %d.\n\n",
                  argv[i], RAZER_MAX_NUM_DEVICES);
                return -1;
            }
            /* 0 keeps the default meaning: apply to all devices. */
            settings.device = (uint8_t) device_id;

        } else if (!strcmp(argv[i], "--stage")) {

            CHECK_ARGS(
              "Can't set active dpi stage. DPI stage number not passed in parameter.\n\n")

            uint64_t stage = 0;
            if (parse_uint_strict(argv[i], &stage) || stage < 1
                || stage > MAX_DPI_STAGES) {
                fprintf(
                  stderr,
                  "Incorrect active DPI stage: %s. Must be between 1 and MAX_DPI_STAGES = %d\n\n",
                  argv[i], MAX_DPI_STAGES);
                return -1;
            }
            settings.active_stage = (uint8_t) stage;
            settings.set_active_stage = 1;

        } else if (!strcmp(argv[i], "--polling-rate")) {

            CHECK_ARGS(
              "Can't set polling rate. Polling rate value not passed in parameter.\n\n")

            uint64_t polling_rate = 0;
            if (parse_uint_strict(argv[i], &polling_rate)
                || polling_rate > UINT16_MAX
                || !polling_rate_supported((uint16_t) polling_rate)) {
                fprintf(
                  stderr,
                  "Can't set unknown polling rate value: %s. Available values: "
                  "125, 250, 500, 1000, 2000, 4000, 8000\n\n",
                  argv[i]);
                return -1;
            }
            settings.polling_rate = (uint16_t) polling_rate;
            settings.set_polling_rate = 1;

        } else if (!strcmp(argv[i], "--dpi")) {

            CHECK_ARGS("Can't set DPI. DPI value not passed in parameter.\n\n")

            params = razer_strdup(argv[i]);
            if (!params) {
                fprintf(stderr, "Can't set DPI: out of memory.\n\n");
                return -1;
            }

            if (parse_dpi_value(params, &settings.dpi.x, &settings.dpi.y)) {
                fprintf(
                  stderr,
                  "Incorrect DPI value: %s. Format: N or NxN, %d <= DPI <= %d\n\n",
                  argv[i], DPI_MIN, DPI_MAX);
                free(params);
                return -1;
            }

            free(params);
            settings.set_dpi = 1;

        } else if (!strcmp(argv[i], "--dpi-stages")) {

            CHECK_ARGS(
              "Can't set DPI stages. DPI stages not passed in parameter.\n\n")

            params = razer_strdup(argv[i]);
            if (!params) {
                fprintf(stderr, "Can't set DPI stages: out of memory.\n\n");
                return -1;
            }

            char *cursor = params;
            while (*cursor) {
                char *next = strchr(cursor, ',');

                if (next)
                    *next++ = '\0';

                if (settings.stages_count >= MAX_DPI_STAGES) {
                    fprintf(stderr,
                            "Can't set DPI stages: too many stages in %s. "
                            "Max stages count is %d\n\n",
                            argv[i], MAX_DPI_STAGES);
                    free(params);
                    return -1;
                }

                struct mouse_dpi *stage
                  = &settings.dpi_stages[settings.stages_count];
                if (parse_dpi_value(cursor, &stage->x, &stage->y)) {
                    fprintf(
                      stderr,
                      "Can't set DPI stages: incorrect value \"%s\" in %s. "
                      "Format: N or NxN, %d <= DPI <= %d, comma separated\n",
                      cursor, argv[i], DPI_MIN, DPI_MAX);
                    free(params);
                    return -1;
                }

                settings.stages_count++;

                if (!next)
                    break;
                cursor = next;
            }

            free(params);

            if (settings.stages_count == 0) {
                fprintf(stderr,
                        "Can't set DPI stages: empty stages list in %s.\n\n",
                        argv[i]);
                return -1;
            }

            settings.set_dpi_stages = 1;

        } else if (!strcmp(argv[i], "--idle-time")) {

            CHECK_ARGS(
              "Can't set idle time. Idle time value not passed in parameter.\n\n")

            uint64_t idle_time = 0;
            if (parse_uint_strict(argv[i], &idle_time) || idle_time < 60
                || idle_time > 900) {
                fprintf(
                  stderr,
                  "Can't set idle time value: %s. Idle time is in seconds, "
                  "must be between 60-900 seconds.\n\n",
                  argv[i]);
                return -1;
            }
            settings.idle_time = (uint16_t) idle_time;
            settings.set_idle_time = 1;

        } else if (!strcmp(argv[i], "--battery-threshold")) {

            CHECK_ARGS(
              "Can't set low battery threshold. Threshold value not passed in parameter.\n\n")

            uint64_t threshold = 0;
            if (parse_uint_strict(argv[i], &threshold) || threshold < 5
                || threshold > 100) {
                fprintf(
                  stderr,
                  "Can't set low battery threshold: %s. Threshold must be between 5-100%%.\n\n",
                  argv[i]);
                return -1;
            }
            settings.battery_threshold = (uint8_t) threshold;
            settings.set_battery_threshold = 1;

        } else if (!strcmp(argv[i], "--lod")) {

            CHECK_ARGS(
              "Can't set Lift-Off Distance. Value not passed in parameter.\n\n")

            if (!strcmp(argv[i], "low")) {
                settings.lod = RAZER_LOD_LOW;
                settings.set_lod = 1;
            } else if (!strcmp(argv[i], "medium")) {
                settings.lod = RAZER_LOD_MEDIUM;
                settings.set_lod = 1;
            } else if (!strcmp(argv[i], "high")) {
                settings.lod = RAZER_LOD_HIGH;
                settings.set_lod = 1;
            } else {
                fprintf(
                  stderr,
                  "Can't set Lift-Off Distance: %s. Available values: low, medium, high.\n\n",
                  argv[i]);
                return -1;
            }

        } else if (!strcmp(argv[i], "--async-lod")) {

            CHECK_ARGS(
              "Can't set Async Lift-Off Distance. Value not passed in parameter.\n\n")

            params = razer_strdup(argv[i]);
            if (!params) {
                fprintf(
                  stderr,
                  "Can't set Async Lift-Off Distance: out of memory.\n\n");
                return -1;
            }

            char *lod_text = params;
            char *ld_text = strchr(params, 'x');
            if (ld_text) {
                *ld_text++ = '\0';
                /* Reject "12x10x8" style garbage. */
                if (strchr(ld_text, 'x')) {
                    fprintf(
                      stderr,
                      "Can't set Async Lift-Off Distance: %s. Format: LODxLD\n\n",
                      argv[i]);
                    free(params);
                    return -1;
                }
            }

            uint64_t lod = 0;
            uint64_t ld = 0;
            if (parse_uint_strict(lod_text, &lod)) {
                fprintf(
                  stderr,
                  "Can't set Async Lift-Off Distance: %s. Lift-Off Distance must be a number.\n\n",
                  argv[i]);
                free(params);
                return -1;
            }
            if (ld_text) {
                if (parse_uint_strict(ld_text, &ld)) {
                    fprintf(
                      stderr,
                      "Can't set Async Lift-Off Distance: %s. Landing Distance must be a number.\n\n",
                      argv[i]);
                    free(params);
                    return -1;
                }
            } else {
                /* Single value: LD defaults to LOD - 1; when LOD < 2 this
                 * becomes 0 and the range check below reports it. */
                ld = (lod >= 2) ? lod - 1 : 0;
            }

            free(params);

            if (lod > ld && lod >= 2 && lod <= 26 && ld >= 1 && ld <= 25) {
                settings.lod = (uint8_t) lod;
                settings.ld = (uint8_t) ld;
                settings.set_async_lod = 1;
                settings.set_lod = 0;
            } else {
                fprintf(
                  stderr,
                  "Can't set async Lift-Off Distance. Lift-Off Distance = %llu Landing Distance = %llu.\n"
                  "Lift-Off Distance must be between 2-26 and Landing Distance must be between 1-25.\n\n",
                  (unsigned long long) lod, (unsigned long long) ld);
                return -1;
            }

        } else if (strncmp(argv[i], "--", 2) == 0) {

            fprintf(stderr, "Unknown option: %s. Try 'razerctl --help'.\n\n",
                    argv[i]);
            return -1;
        }
    }

#if defined(RAZERCTL_USE_LIBUSB) && defined(RAZERCTL_USE_HIDRAW)
    if (settings.libusb)
        res = razer_libusb_init(&settings);
    else
        res = razer_hidraw_init(&settings);
#elif defined(RAZERCTL_USE_HIDRAW)
    res = razer_hidraw_init(&settings);
#else
    res = razer_libusb_init(&settings);
#endif

    return res;
}
