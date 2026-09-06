#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/ad_version.h"
#include "io/trackdisk/trackdisk.h"

static void usage(const char *program)
{
    printf("Usage:\n");
    printf("  %s --version\n", program);
    printf("  %s probe <unit>\n", program);
    printf("  %s read-sector <unit> <cylinder> <head> <sector>\n", program);
}

static int parse_ulong(const char *text, ULONG *value)
{
    char *end;
    unsigned long parsed;

    if (text == NULL || value == NULL || *text == '\0') {
        return 0;
    }

    parsed = strtoul(text, &end, 10);
    if (*end != '\0') {
        return 0;
    }

    *value = (ULONG)parsed;
    return 1;
}

static int command_probe(ULONG unit)
{
    AdTrackDisk disk;
    AdTdStatus status;
    AdTdResult result;

    result = ad_td_open(&disk, unit);
    if (result != AD_TD_OK) {
        fprintf(stderr, "DF%u: %s\n", (unsigned int)unit, ad_td_result_string(result));
        return 2;
    }

    result = ad_td_get_status(&disk, &status);
    if (result == AD_TD_OK) {
        printf("DF%u: media=%s write-protected=%s\n",
               (unsigned int)unit,
               status.media_present ? "present" : "absent",
               status.write_protected ? "yes" : "no");
    } else {
        fprintf(stderr, "DF%u: %s\n", (unsigned int)unit, ad_td_result_string(result));
    }

    ad_td_close(&disk);
    return result == AD_TD_OK ? 0 : 2;
}

static int command_read_sector(ULONG unit, ULONG cylinder, ULONG head, ULONG sector)
{
    AdTrackDisk disk;
    AdTdResult result;
    unsigned char buffer[AD_TD_SECTOR_SIZE];
    ULONG i;

    result = ad_td_open(&disk, unit);
    if (result != AD_TD_OK) {
        fprintf(stderr, "DF%u: %s\n", (unsigned int)unit, ad_td_result_string(result));
        return 2;
    }

    result = ad_td_read_sector(&disk, cylinder, head, sector, buffer);
    if (result == AD_TD_OK) {
        printf("DF%u C%u H%u S%u read OK\n",
               (unsigned int)unit, (unsigned int)cylinder,
               (unsigned int)head, (unsigned int)sector);
        for (i = 0; i < 16UL; ++i) {
            printf("%02x%c", (unsigned int)buffer[i], i == 15UL ? '\n' : ' ');
        }
    } else {
        fprintf(stderr, "DF%u C%u H%u S%u: %s\n",
                (unsigned int)unit, (unsigned int)cylinder,
                (unsigned int)head, (unsigned int)sector,
                ad_td_result_string(result));
    }

    ad_td_close(&disk);
    return result == AD_TD_OK ? 0 : 2;
}

int main(int argc, char **argv)
{
    ULONG unit;
    ULONG cylinder;
    ULONG head;
    ULONG sector;

    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        puts(ad_version_string());
        puts(AMIDISK_TARGET);
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "probe") == 0 && parse_ulong(argv[2], &unit)) {
        if (unit > 3UL) {
            fprintf(stderr, "unit must be 0..3\n");
            return 1;
        }
        return command_probe(unit);
    }

    if (argc == 6 && strcmp(argv[1], "read-sector") == 0 &&
        parse_ulong(argv[2], &unit) && parse_ulong(argv[3], &cylinder) &&
        parse_ulong(argv[4], &head) && parse_ulong(argv[5], &sector)) {
        if (unit > 3UL) {
            fprintf(stderr, "unit must be 0..3\n");
            return 1;
        }
        return command_read_sector(unit, cylinder, head, sector);
    }

    usage(argv[0]);
    return 1;
}
