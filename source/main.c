#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/statvfs.h>

#include <xenos/xenos.h>
#include <console/console.h>
#include <input/input.h>
#include <usb/usbmain.h>
#include <libfat/fat.h>
#include <diskio/ata.h>
#include <xenon_smc/xenon_smc.h>
#include <xb360/xb360.h>
#include <xenon_sfcx/xenon_sfcx.h>

int bdev_enum(int handle, const char **name);

#define MAX_DEVICES 16
#define REPORT_SIZE 32768
#define RTC_EPOCH_UNIX 1005782400LL

typedef struct {
    char name[32];
    uint64_t total, free_bytes, used;
    uint32_t used_pct;
    int valid;
} StorageInfo;

static StorageInfo storages[MAX_DEVICES];
static int storage_count = 0;
static int fahrenheit = 0;

static const char *board_name(int t) {
    switch (t) {
        case REV_XENON: return "Xenon";
        case REV_ZEPHYR: return "Zephyr";
        case REV_FALCON: return "Falcon";
        case REV_JASPER: return "Jasper";
        case REV_TRINITY: return "Trinity";
        case REV_CORONA: return "Corona";
        case REV_CORONA_PHISON: return "Corona (Phison)";
        case REV_WINCHESTER: return "Winchester";
        case REV_WINCHESTER_MMC: return "Winchester (MMC)";
        default: return "N/A";
    }
}

static const char *tray_name(uint8_t s) {
    switch (s) {
        case 0x60: return "Open";
        case 0x62: return "Closed";
        case 0x63: return "Opening";
        case 0x64: return "Closing";
        case 0x65: return "Error";
        default: return "N/A";
    }
}

static const char *avpack_name(uint8_t v) {
    switch (v & 0x1c) {
        case 0x1c: return "Nothing";
        case 0x18: return "VGA";
        case 0x14: return "Composite";
        case 0x10: return "TOSLINK/RCA audio";
        case 0x0c: return "Component (HDTV)";
        case 0x08: return "S-video";
        case 0x04: return "SCART/RGB";
        case 0x00: return "Component (TV)";
        default: return "N/A";
    }
}

static double temp_display(uint16_t raw) {
    double c = (double)raw / 256.0;
    return fahrenheit ? (c * 9.0 / 5.0 + 32.0) : c;
}

static void smc_command(uint8_t command, uint8_t *reply) {
    memset(reply, 0, 16);
    reply[0] = command;
    xenon_smc_send_message(reply);
    xenon_smc_receive_response(reply);
}

static void refresh_storages(void) {
    int handle = -1;
    const char *name;
    storage_count = 0;
    memset(storages, 0, sizeof(storages));

    while (storage_count < MAX_DEVICES) {
        handle = bdev_enum(handle, &name);
        if (handle < 0) break;
        if (!name) continue;

        StorageInfo *s = &storages[storage_count];
        snprintf(s->name, sizeof(s->name), "%s", name);

        char path[64];
        struct statvfs v;
        snprintf(path, sizeof(path), "%s:/", name);
        if (statvfs(path, &v) == 0) {
            s->total = (uint64_t)v.f_blocks * (uint64_t)v.f_frsize;
            s->free_bytes = (uint64_t)v.f_bfree * (uint64_t)v.f_frsize;
            if (s->free_bytes > s->total) s->free_bytes = s->total;
            s->used = s->total - s->free_bytes;
            s->used_pct = s->total ? (uint32_t)((s->used * 100ULL) / s->total) : 0;
            s->valid = 1;
        }
        ++storage_count;
    }
}

static void refresh_all(void) {
    refresh_storages();
    sfcx_init();
}

static void print_rtc(void) {
    uint8_t r[16];
    smc_command(0x04, r);
    if (r[0] != 0x04 || !(r[6] & 1)) {
        printf("RTC / date-time       N/A\n");
        return;
    }

    uint64_t ms = (uint64_t)r[1] |
                  ((uint64_t)r[2] << 8) |
                  ((uint64_t)r[3] << 16) |
                  ((uint64_t)r[4] << 24) |
                  ((uint64_t)r[5] << 32);
    time_t t = (time_t)(RTC_EPOCH_UNIX + ms / 1000ULL);
    struct tm *tmv = gmtime(&t);
    if (!tmv) {
        printf("RTC / date-time       N/A\n");
        return;
    }
    printf("RTC / date-time       %04d-%02d-%02d %02d:%02d:%02d UTC\n",
           tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday,
           tmv->tm_hour, tmv->tm_min, tmv->tm_sec);
}

static void draw(void) {
    uint8_t sensors[16], tray[16], smc[16];
    uint8_t av = (uint8_t)xenon_smc_read_avpack();

    smc_command(0x07, sensors);
    smc_command(0x0A, tray);
    smc_command(0x12, smc);

    console_clrscr();
    printf("X3SI - Xbox 360 System Information\n");
    printf("==================================\n");
    printf("A: %s    X: Save    Y: Refresh    B: Exit\n\n",
           fahrenheit ? "Use Celsius" : "Use Fahrenheit");

    printf("[Hardware]\n");
    printf("Motherboard           %s\n", board_name(xenon_get_console_type()));
    printf("CPU PVR               0x%08X\n", xenon_get_CPU_PVR());
    printf("Xenos GPU ID          0x%08X\n", xenon_get_XenosID());
    printf("PCI bridge revision   0x%02X\n", xenon_get_PCIBridgeRevisionID());
    printf("System RAM             %llu bytes\n",
           (unsigned long long)xenon_get_ram_size());

    printf("\n[Thermal / cooling]\n");
    if (sensors[0] == 0x07) {
        printf("CPU temperature       %.2f %s\n", temp_display((sensors[2] << 8) | sensors[1]), fahrenheit ? "F" : "C");
        printf("GPU temperature       %.2f %s\n", temp_display((sensors[4] << 8) | sensors[3]), fahrenheit ? "F" : "C");
        printf("eDRAM temperature     %.2f %s\n", temp_display((sensors[6] << 8) | sensors[5]), fahrenheit ? "F" : "C");
        printf("Chassis temperature   %.2f %s\n", temp_display((sensors[8] << 8) | sensors[7]), fahrenheit ? "F" : "C");
        printf("Fan target            %u%%\n", sensors[9]);
    } else {
        printf("CPU temperature       N/A\nGPU temperature       N/A\neDRAM temperature     N/A\nChassis temperature   N/A\nFan target            N/A\n");
    }

    printf("\n[System / firmware]\n");
    print_rtc();
    printf("Kernel/dashboard      N/A (bare-metal API)\n");
    if (smc[0] == 0x12)
        printf("SMC version           %u.%u type 0x%02X\n", smc[2], smc[3], smc[1]);
    else
        printf("SMC version           N/A\n");

    printf("\n[DVD / A/V]\n");
    printf("DVD tray              %s (0x%02X)\n", tray_name(tray[1]), tray[1]);
    printf("A/V pack              %s (0x%02X)\n", avpack_name(av), av);
    printf("A/V raw flags         0x%02X\n", av);

    printf("\n[NAND]\n");
    if (sfc.size_bytes)
        printf("Raw NAND size         %d bytes (%d MiB)\nUsable FS size       %d bytes\n",
               sfc.size_bytes, sfc.size_mb, sfc.size_usable_fs);
    else
        printf("Raw NAND size         N/A\nUsable FS size       N/A\n");
    printf("KeyVault size         0x%08X bytes\n", xenon_get_kv_size());
    printf("KeyVault offset       0x%08X\n", xenon_get_kv_offset());

    printf("\n[Storage devices]\n");
    if (!storage_count) {
        printf("No mounted storage devices detected.\n");
    } else {
        int i;
        uint64_t gt = 0, gu = 0, gf = 0;
        for (i = 0; i < storage_count; ++i) {
            StorageInfo *s = &storages[i];
            printf("%s:/\n", s->name);
            if (!s->valid) {
                printf("  Total capacity      N/A\n  Used                N/A\n  Free                N/A\n");
                continue;
            }
            printf("  Total capacity      %llu bytes\n", (unsigned long long)s->total);
            printf("  Used                %llu bytes (%u%%)\n", (unsigned long long)s->used, s->used_pct);
            printf("  Free                %llu bytes (%u%%)\n", (unsigned long long)s->free_bytes, 100U - s->used_pct);
            gt += s->total; gu += s->used; gf += s->free_bytes;
        }
        printf("Grand total capacity  %llu bytes\n", (unsigned long long)gt);
        printf("Grand total used      %llu bytes\n", (unsigned long long)gu);
        printf("Grand total free      %llu bytes\n", (unsigned long long)gf);
    }

    printf("\nUnknown/unavailable values are N/A; nothing is hard-coded.\n");
}

static void report_stamp(char *out, size_t n) {
    uint8_t r[16];
    smc_command(0x04, r);
    if (r[0] == 0x04 && (r[6] & 1)) {
        uint64_t ms = (uint64_t)r[1] |
                      ((uint64_t)r[2] << 8) |
                      ((uint64_t)r[3] << 16) |
                      ((uint64_t)r[4] << 24) |
                      ((uint64_t)r[5] << 32);
        time_t t = (time_t)(RTC_EPOCH_UNIX + ms / 1000ULL);
        struct tm *tmv = gmtime(&t);
        if (tmv) {
            snprintf(out, n, "%04d-%02d-%02d_%02d-%02d-%02d",
                     tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday,
                     tmv->tm_hour, tmv->tm_min, tmv->tm_sec);
            return;
        }
    }
    snprintf(out, n, "RTC-unknown");
}

static int build_report(char *buf, size_t n) {
    int used = 0, i;
    char stamp[64];
    report_stamp(stamp, sizeof(stamp));

    used += snprintf(buf + used, n - (size_t)used,
        "X3SI - Xbox 360 System Information\r\nTimestamp: %s\r\nTemperature unit: %s\r\n\r\n",
        stamp, fahrenheit ? "Fahrenheit" : "Celsius");

    used += snprintf(buf + used, n - (size_t)used,
        "[Hardware]\r\nMotherboard: %s\r\nCPU PVR: 0x%08X\r\nXenos GPU ID: 0x%08X\r\nPCI bridge revision: 0x%02X\r\nSystem RAM: %llu bytes\r\n\r\n",
        board_name(xenon_get_console_type()), xenon_get_CPU_PVR(), xenon_get_XenosID(),
        xenon_get_PCIBridgeRevisionID(), (unsigned long long)xenon_get_ram_size());

    {
        uint8_t s[16];
        smc_command(0x07, s);
        if (s[0] == 0x07) {
            used += snprintf(buf + used, n - (size_t)used,
                "CPU temperature: %.2f %s\r\nGPU temperature: %.2f %s\r\neDRAM temperature: %.2f %s\r\nChassis temperature: %.2f %s\r\nFan target: %u%%\r\n",
                temp_display((s[2] << 8) | s[1]), fahrenheit ? "F" : "C",
                temp_display((s[4] << 8) | s[3]), fahrenheit ? "F" : "C",
                temp_display((s[6] << 8) | s[5]), fahrenheit ? "F" : "C",
                temp_display((s[8] << 8) | s[7]), fahrenheit ? "F" : "C",
                s[9]);
        } else {
            used += snprintf(buf + used, n - (size_t)used,
                "CPU temperature: N/A\r\nGPU temperature: N/A\r\neDRAM temperature: N/A\r\nChassis temperature: N/A\r\nFan target: N/A\r\n");
        }
    }

    used += snprintf(buf + used, n - (size_t)used, "\r\n[System / firmware]\r\n");
    {
        uint8_t r[16], s[16];
        smc_command(0x04, r);
        smc_command(0x12, s);
        if (r[0] == 0x04 && (r[6] & 1)) {
            uint64_t ms = (uint64_t)r[1] | ((uint64_t)r[2] << 8) |
                          ((uint64_t)r[3] << 16) | ((uint64_t)r[4] << 24) |
                          ((uint64_t)r[5] << 32);
            time_t t = (time_t)(RTC_EPOCH_UNIX + ms / 1000ULL);
            struct tm *tmv = gmtime(&t);
            if (tmv)
                used += snprintf(buf + used, n - (size_t)used,
                    "RTC/date-time: %04d-%02d-%02d %02d:%02d:%02d UTC\r\n",
                    tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday,
                    tmv->tm_hour, tmv->tm_min, tmv->tm_sec);
            else used += snprintf(buf + used, n - (size_t)used, "RTC/date-time: N/A\r\n");
        } else used += snprintf(buf + used, n - (size_t)used, "RTC/date-time: N/A\r\n");
        if (s[0] == 0x12)
            used += snprintf(buf + used, n - (size_t)used, "SMC version: %u.%u type 0x%02X\r\n", s[2], s[3], s[1]);
        else used += snprintf(buf + used, n - (size_t)used, "SMC version: N/A\r\n");
    }
    used += snprintf(buf + used, n - (size_t)used, "Kernel/dashboard: N/A (bare-metal API)\r\n");

    {
        uint8_t tray[16];
        uint8_t av = (uint8_t)xenon_smc_read_avpack();
        smc_command(0x0A, tray);
        used += snprintf(buf + used, n - (size_t)used,
            "\r\n[DVD / A/V]\r\nDVD tray: %s (0x%02X)\r\nA/V pack: %s (0x%02X)\r\nA/V raw flags: 0x%02X\r\n",
            tray_name(tray[1]), tray[1], avpack_name(av), av, av);
    }

    used += snprintf(buf + used, n - (size_t)used, "\r\n[NAND]\r\n");
    if (sfc.size_bytes)
        used += snprintf(buf + used, n - (size_t)used,
            "Raw NAND size: %d bytes\r\nUsable FS size: %d bytes\r\n", sfc.size_bytes, sfc.size_usable_fs);
    else used += snprintf(buf + used, n - (size_t)used, "Raw NAND size: N/A\r\nUsable FS size: N/A\r\n");
    used += snprintf(buf + used, n - (size_t)used,
        "KeyVault size: 0x%08X bytes\r\nKeyVault offset: 0x%08X\r\n", xenon_get_kv_size(), xenon_get_kv_offset());

    used += snprintf(buf + used, n - (size_t)used, "\r\n[Storage devices]\r\n");
    for (i = 0; i < storage_count; ++i) {
        StorageInfo *s = &storages[i];
        used += snprintf(buf + used, n - (size_t)used, "%s:/\r\n", s->name);
        if (s->valid)
            used += snprintf(buf + used, n - (size_t)used,
                "Total capacity: %llu bytes\r\nUsed: %llu bytes (%u%%)\r\nFree: %llu bytes (%u%%)\r\n",
                (unsigned long long)s->total, (unsigned long long)s->used, s->used_pct,
                (unsigned long long)s->free_bytes, 100U - s->used_pct);
        else
            used += snprintf(buf + used, n - (size_t)used, "Total capacity: N/A\r\nUsed: N/A\r\nFree: N/A\r\n");
    }
    {
        uint64_t total = 0, used_bytes = 0, free_bytes = 0;
        for (i = 0; i < storage_count; ++i) if (storages[i].valid) {
            total += storages[i].total; used_bytes += storages[i].used; free_bytes += storages[i].free_bytes;
        }
        used += snprintf(buf + used, n - (size_t)used,
            "Grand total capacity: %llu bytes\r\nGrand total used: %llu bytes\r\nGrand total free: %llu bytes\r\n",
            (unsigned long long)total, (unsigned long long)used_bytes, (unsigned long long)free_bytes);
    }
    return used < 0 ? -1 : used;
}

static int choose_storage(void) {
    struct controller_data_s p, old;
    int selected = 0;
    memset(&old, 0, sizeof(old));
    if (storage_count <= 0) return -1;

    for (;;) {
        console_clrscr();
        printf("X3SI storage selector\n=====================\n");
        printf("D-pad: choose    A: select    B: cancel\n\n");
        {
            int i;
            for (i = 0; i < storage_count; ++i)
                printf("%c %s:/\n", i == selected ? '>' : ' ', storages[i].name);
        }
        usb_do_poll();
        get_controller_data(&p, 0);
        if (p.b && !old.b) return -1;
        if (p.up && !old.up) { if (--selected < 0) selected = storage_count - 1; }
        if (p.down && !old.down) { if (++selected >= storage_count) selected = 0; }
        if (p.a && !old.a) return selected;
        old = p;
        mdelay(30);
    }
}

static int save_report(void) {
    char report[REPORT_SIZE], stamp[64], path[128];
    int selected = choose_storage();
    if (selected < 0) return 0;
    report_stamp(stamp, sizeof(stamp));
    if (build_report(report, sizeof(report)) < 0) return -1;
    snprintf(path, sizeof(path), "%s:/SystemInfo_%s.txt", storages[selected].name, stamp);

    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t len = strlen(report);
    if (fwrite(report, 1, len, f) != len) { fclose(f); return -1; }
    fclose(f);
    return 1;
}

int main(void) {
    struct controller_data_s p, old;
    xenos_init(VIDEO_MODE_AUTO);
    console_init();
    xenon_make_it_faster(XENON_SPEED_FULL);
    usb_init();
    usb_do_poll();
    xenon_ata_init();
    xenon_atapi_init();
    fatInitDefault();
    memset(&old, 0, sizeof(old));
    refresh_all();

    for (;;) {
        draw();
        usb_do_poll();
        get_controller_data(&p, 0);

        if (p.logo && !old.logo) return 0;
        if (p.a && !old.a) fahrenheit = !fahrenheit;
        if (p.y && !old.y) refresh_all();
        if (p.x && !old.x) {
            int r = save_report();
            console_clrscr();
            printf(r > 0 ? "Report saved successfully.\n" : (r < 0 ? "Could not save report.\n" : "Save cancelled.\n"));
            mdelay(900);
        }
        if (p.b && !old.b) return 0;

        old = p;
        mdelay(100);
    }
    return 0;
}
