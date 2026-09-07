#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/ad_version.h"
#include "io/adf/adf.h"
#include "io/trackdisk/trackdisk.h"
#include "operations/image_adf.h"

static void usage(const char *program)
{
    printf("Usage:\n");
    printf("  %s --version\n", program);
    printf("  %s probe <unit>\n", program);
    printf("  %s read-sector <unit> <cylinder> <head> <sector>\n", program);
    printf("  %s adf-info <path>\n", program);
    printf("  %s adf-read-sector <path> <cylinder> <head> <sector>\n", program);
    printf("  %s image-adf <unit> <path>\n", program);
    printf("  %s qualify-media-change <unit>\n", program);
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

static void print_prefix16(const unsigned char *buffer)
{
    ULONG i;

    for (i = 0; i < 16UL; ++i) {
        printf("%02x%c", (unsigned int)buffer[i], i == 15UL ? '\n' : ' ');
    }
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
        print_prefix16(buffer);
    } else {
        fprintf(stderr, "DF%u C%u H%u S%u: %s\n",
                (unsigned int)unit, (unsigned int)cylinder,
                (unsigned int)head, (unsigned int)sector,
                ad_td_result_string(result));
    }

    ad_td_close(&disk);
    return result == AD_TD_OK ? 0 : 2;
}

static int command_adf_info(const char *path)
{
    AdAdfImage image;
    AdAdfResult result;

    result = ad_adf_open(&image, path);
    if (result != AD_ADF_OK) {
        fprintf(stderr, "%s: %s\n", path, ad_adf_result_string(result));
        return 2;
    }

    printf("ADF: %s\n", path);
    printf("size=%u bytes cylinders=%u heads=%u sectors/track=%u sector-size=%u\n",
           (unsigned int)image.size_bytes,
           (unsigned int)AD_ADF_CYLINDERS,
           (unsigned int)AD_ADF_HEADS,
           (unsigned int)AD_ADF_SECTORS_PER_TRACK,
           (unsigned int)AD_ADF_SECTOR_SIZE);
    ad_adf_close(&image);
    return 0;
}

static int command_adf_read_sector(const char *path, ULONG cylinder,
                                   ULONG head, ULONG sector)
{
    AdAdfImage image;
    AdAdfResult result;
    unsigned char buffer[AD_ADF_SECTOR_SIZE];

    result = ad_adf_open(&image, path);
    if (result != AD_ADF_OK) {
        fprintf(stderr, "%s: %s\n", path, ad_adf_result_string(result));
        return 2;
    }

    result = ad_adf_read_sector(&image, cylinder, head, sector, buffer);
    if (result == AD_ADF_OK) {
        printf("ADF %s C%u H%u S%u read OK\n", path,
               (unsigned int)cylinder, (unsigned int)head,
               (unsigned int)sector);
        print_prefix16(buffer);
    } else {
        fprintf(stderr, "ADF %s C%u H%u S%u: %s\n", path,
                (unsigned int)cylinder, (unsigned int)head,
                (unsigned int)sector, ad_adf_result_string(result));
    }

    ad_adf_close(&image);
    return result == AD_ADF_OK ? 0 : 2;
}

static int command_image_adf(ULONG unit, const char *path)
{
    AdImageReport report;
    AdImageResult result;

    printf("Imaging DF%u: -> %s\n", (unsigned int)unit, path);
    result = ad_image_disk_to_adf(unit, path, &report);
    if (result != AD_IMAGE_OK) {
        fprintf(stderr, "image-adf failed: %s", ad_image_result_string(result));
        if (report.source_result != AD_TD_OK) {
            fprintf(stderr, " (%s)", ad_td_result_string(report.source_result));
        }
        fputc('\n', stderr);
        return 2;
    }

    printf("image-adf OK: sectors=%u bytes=%u change=%u\n",
           (unsigned int)report.sectors_written,
           (unsigned int)report.bytes_written,
           (unsigned int)report.end_change_number);
    return 0;
}

static int command_qualify_media_change(ULONG unit)
{
    AdTrackDisk disk;
    AdTdStatus before_status;
    AdTdStatus after_status;
    AdTdResult result;
    ULONG before_change;
    ULONG after_change;
    int ch;

    result = ad_td_open(&disk, unit);
    if (result != AD_TD_OK) {
        fprintf(stderr, "DF%u: %s\n", (unsigned int)unit, ad_td_result_string(result));
        return 2;
    }

    result = ad_td_get_status(&disk, &before_status);
    if (result != AD_TD_OK) {
        fprintf(stderr, "DF%u initial status: %s\n", (unsigned int)unit,
                ad_td_result_string(result));
        ad_td_close(&disk);
        return 2;
    }

    result = ad_td_get_change_number(&disk, &before_change);
    if (result != AD_TD_OK) {
        fprintf(stderr, "DF%u initial change number: %s\n", (unsigned int)unit,
                ad_td_result_string(result));
        ad_td_close(&disk);
        return 2;
    }

    printf("DF%u before: media=%s change=%u\n", (unsigned int)unit,
           before_status.media_present ? "present" : "absent",
           (unsigned int)before_change);
    puts("Eject or swap the disk in FS-UAE now, then press RETURN.");
    do {
        ch = getchar();
    } while (ch != '\n' && ch != EOF);

    result = ad_td_get_status(&disk, &after_status);
    if (result != AD_TD_OK) {
        fprintf(stderr, "DF%u final status: %s\n", (unsigned int)unit,
                ad_td_result_string(result));
        ad_td_close(&disk);
        return 2;
    }

    result = ad_td_get_change_number(&disk, &after_change);
    if (result != AD_TD_OK) {
        fprintf(stderr, "DF%u final change number: %s\n", (unsigned int)unit,
                ad_td_result_string(result));
        ad_td_close(&disk);
        return 2;
    }

    printf("DF%u after: media=%s change=%u\n", (unsigned int)unit,
           after_status.media_present ? "present" : "absent",
           (unsigned int)after_change);

    ad_td_close(&disk);

    if (after_change == before_change) {
        fprintf(stderr, "qualification failed: change number did not change\n");
        return 2;
    }

    puts("qualification OK: trackdisk media change observed");
    return 0;
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

    if (argc == 3 && strcmp(argv[1], "adf-info") == 0) {
        return command_adf_info(argv[2]);
    }

    if (argc == 6 && strcmp(argv[1], "adf-read-sector") == 0 &&
        parse_ulong(argv[3], &cylinder) && parse_ulong(argv[4], &head) &&
        parse_ulong(argv[5], &sector)) {
        return command_adf_read_sector(argv[2], cylinder, head, sector);
    }

    if (argc == 4 && strcmp(argv[1], "image-adf") == 0 && parse_ulong(argv[2], &unit)) {
        if (unit > 3UL) {
            fprintf(stderr, "unit must be 0..3\n");
            return 1;
        }
        return command_image_adf(unit, argv[3]);
    }

    if (argc == 3 && strcmp(argv[1], "qualify-media-change") == 0 &&
        parse_ulong(argv[2], &unit)) {
        if (unit > 3UL) {
            fprintf(stderr, "unit must be 0..3\n");
            return 1;
        }
        return command_qualify_media_change(unit);
    }

    usage(argv[0]);
    return 1;
}
