#include "razer.h"

#ifdef RAZERCTL_USE_LIBUSB
#include "libusb.h"
#endif
#ifdef RAZERCTL_USE_HIDRAW
#include "hidraw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_ARGS(msg)                                   \
    i++;                                                  \
    if (i >= argc || !argv[i] || strstr(argv[i], "--")) { \
        printf(msg);                                      \
        return -1;                                        \
    }

int main(int argc, char **argv)
{
    int res = 0;
    char *params = NULL;
    char *token = NULL;
    char *dpi_stages[MAX_DPI_STAGES] = { 0 };
    struct razerctl_settings settings = { 0 };

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
#if defined(RAZERCTL_USE_LIBUSB) && defined(RAZERCTL_USE_HIDRAW)
            printf("Usage: razerctl [OPTION]...\n"
                   "  --help\t\tPrint this help and exit\n"
                   "  --json\t\tOutput format JSON\n"
                   "  --libusb\t\tUse libusb instead HIDRAW\n"
                   "  --devices\t\tPrint only device list\n"
                   "  --lod\t\t\tSet Lift-Off Distance. Avaible values: low, medium, high.\n"
                   "  --async-lod\t\tSet Async Lift-Off Distance. Format: Lift-Off Distance x Landing Distance."
                   " 2 <= LOD <= 26 and 1 <= LD <= 25. LOD > LD. Example: 12x10\n"
                   "  --device\t\tDevice ID. If not set then all devices\n"
                   "  --polling-rate\tSet polling rate. Available values: 125, 250, 500, 1000, 2000, 4000, 8000\n"
                   "  --dpi\t\t\tSet DPI. Examples: 800 - DPI x=800 y=800, 800x1200 - DPI x=800 y=1200\n"
                   "  --idle-time\t\tSet idle time for powersave in seconds. Must be between 60-900\n"
                   "  --battery-threshold\tSet low battery charge %% threshold for pawer-save on. Example: 0 - 100\n"
                   "  --stage\t\tSet active DPI stage. Example: 2\n"
                   "  --dpi-stages\t\tSet DPI stages. Example: 800,1600,3200 or 800x800,1600x1600,3200x3200\n\n");
#else
            printf("Usage: razerctl [OPTION]...\n"
                   "  --help\t\tPrint this help and exit\n"
                   "  --json\t\tOutput format JSON\n"
                   "  --devices\t\tPrint only device list\n"
                   "  --lod\t\t\tSet Lift-Off Distance. Avaible values: low, medium, high.\n"
                   "  --async-lod\t\tSet Async Lift-Off Distance. Format: Lift-Off Distance x Landing Distance."
                   " 2 <= LOD <= 26 and 1 <= LD <= 25. LOD > LD. Example: 12x10\n"
                   "  --device\t\tDevice ID. If not set then all devices\n"
                   "  --polling-rate\tSet polling rate. Available values: 125, 250, 500, 1000, 2000, 4000, 8000\n"
                   "  --dpi\t\t\tSet DPI. Examples: 800 - DPI x=800 y=800, 800x1200 - DPI x=800 y=1200\n"
                   "  --idle-time\t\tSet idle time for powersave in seconds. Must be between 60-900\n"
                   "  --battery-threshold\tSet low battery charge %% threshold for pawer-save on. Example: 0 - 100\n"
                   "  --stage\t\tSet active DPI stage. Example: 2\n"
                   "  --dpi-stages\t\tSet DPI stages. Example: 800,1600,3200 or 800x800,1600x1600,3200x3200\n\n");
#endif
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

            CHECK_ARGS("Can't set device id. Device ID not passed in parameter.\n\n")
            settings.device = (uint8_t) strtoul(argv[i], NULL, 0);

        } else if (!strcmp(argv[i], "--stage")) {

            CHECK_ARGS("Can't set active dpi stage. DPI stage number not passed in parameter.\n\n")
            settings.active_stage = (uint8_t) strtoul(argv[i], NULL, 0);

            if (settings.active_stage > MAX_DPI_STAGES) {
                printf("Active DPI stage %hhu greater than MAX_DPI_STAGES = %hu\n", settings.active_stage,
                       MAX_DPI_STAGES);
                return -1;
            }

            settings.set_active_stage = 1;

        } else if (!strcmp(argv[i], "--polling-rate")) {

            CHECK_ARGS("Can't set polling rate. Polling rate value not passed in parameter.\n\n")
            settings.polling_rate = (uint16_t) strtoul(argv[i], NULL, 0);
            settings.set_polling_rate = 1;

        } else if (!strcmp(argv[i], "--dpi")) {

            CHECK_ARGS("Can't set DPI. DPI value not passed in parameter.\n\n")

            if (strchr(argv[i], 'x') != NULL) {

                params = razer_strdup(argv[i]);
                token = strtok(params, "x");

                settings.dpi.x = (uint16_t) strtoul(token, NULL, 0);

                token = strtok(NULL, "x");

                if (token) {
                    settings.dpi.y = (uint16_t) strtoul(token, NULL, 0);
                } else {
                    printf("DPI Y value is't in parameters!\n");
                    free(params);
                    return -1;
                }

                free(params);

            } else {
                settings.dpi.x = (uint16_t) strtoul(argv[i], NULL, 0);
                settings.dpi.y = settings.dpi.x;
            }

            if (settings.dpi.x <= 0 || settings.dpi.y <= 0) {
                printf("Incorrect DPI value: %hux%hu\n", settings.dpi.x, settings.dpi.y);
                return -1;
            }

            settings.set_dpi = 1;

        } else if (!strcmp(argv[i], "--dpi-stages")) {

            CHECK_ARGS("Can't set DPI stages. DPI stages not passed in parameter.\n\n")

            params = razer_strdup(argv[i]);
            token = strtok(params, ",");

            while (token != NULL && settings.stages_count < MAX_DPI_STAGES) {
                dpi_stages[settings.stages_count] = token;
                settings.stages_count++;
                token = strtok(NULL, ",");
            }

            for (uint16_t k = 0; k < settings.stages_count; k++) {
                if (strchr(dpi_stages[k], 'x') != NULL) {
                    token = strtok(dpi_stages[k], "x");
                    settings.dpi_stages[k].x = (uint16_t) strtoul(token, NULL, 0);
                    token = strtok(NULL, "x");
                    if (token) {
                        settings.dpi_stages[k].y = (uint16_t) strtoul(token, NULL, 0);
                    } else {
                        printf("DPI Y value is't in parameters!\n");
                        free(params);
                        return -1;
                    }
                } else {
                    settings.dpi_stages[k].x = (uint16_t) strtoul(dpi_stages[k], NULL, 0);
                    settings.dpi_stages[k].y = settings.dpi_stages[k].x;
                }
            }

            free(params);

            settings.set_dpi_stages = 1;

        } else if (!strcmp(argv[i], "--idle-time")) {

            CHECK_ARGS("Can't set idle time. Idle time value not passed in parameter.\n\n")
            settings.idle_time = (uint16_t) strtoul(argv[i], NULL, 0);

            if (settings.idle_time >= 60 && settings.idle_time <= 900) {
                settings.set_idle_time = 1;
            } else {
                printf("Can't set idle time value: %d. Idle time is in "
                       "seconds, must be between 60-900 seconds.\n\n",
                       settings.idle_time);
                return -1;
            }

        } else if (!strcmp(argv[i], "--battery-threshold")) {

            CHECK_ARGS("Can't set low battery threshold. Threshold value not passed in parameter.\n\n")
            settings.battery_threshold = (uint8_t) strtoul(argv[i], NULL, 0);

            if (settings.battery_threshold >= 5 && settings.battery_threshold <= 100) {
                settings.set_battery_threshold = 1;
            } else {
                printf("Can't set low battery threshold: %d. Threshold must be between 5-100%%.\n\n",
                       settings.battery_threshold);
                return -1;
            }

        } else if (!strcmp(argv[i], "--lod")) {

            CHECK_ARGS("Can't set Lift-Off Distance. Value not passed in parameter.\n\n")

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
                printf("Can't set Lift-Off Distance: %s. Avaible values: low, medium, high.\n\n", argv[i]);
                return -1;
            }

        } else if (!strcmp(argv[i], "--async-lod")) {

            CHECK_ARGS("Can't set Async Lift-Off Distance. Value not passed in parameter.\n\n")

            if (strchr(argv[i], 'x') != NULL) {

                params = razer_strdup(argv[i]);
                token = strtok(params, "x");

                settings.lod = (uint8_t) strtoul(token, NULL, 0);

                token = strtok(NULL, "x");

                if (token) {
                    settings.ld = (uint8_t) strtoul(token, NULL, 0);
                } else {
                    settings.ld = settings.lod - 1;
                }

                free(params);

            } else {
                settings.lod = (uint8_t) strtoul(argv[i], NULL, 0);
                settings.ld = settings.lod - 1;
            }

            if (settings.lod > settings.ld && settings.lod >= 2 && settings.lod <= 26 && settings.ld >= 1
                && settings.ld <= 25) {
                settings.set_async_lod = 1;
                settings.set_lod = 0;
            } else {
                printf("Can't set async Lift-Off Distance. Lift-Off Distance = %d Landing Distance = %d.\n"
                       "Lift-Off Distance must be between 2-26 and Landing Distance must be between 1-25.\n\n",
                       settings.lod, settings.ld);
                return -1;
            }
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
