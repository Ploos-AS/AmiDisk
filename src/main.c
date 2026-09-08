#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/ad_version.h"
#include "io/adf/adf.h"
#include "io/trackdisk/trackdisk.h"
#include "operations/copy_disk.h"
#include "operations/image_adf.h"
#include "operations/recovery_image.h"
#include "operations/recovery_read.h"
#include "operations/restore_adf.h"
#include "operations/restore_preflight.h"
#include "operations/verify_adf.h"

static void usage(const char *program)
{
    printf("Usage:\n");
    printf("  %s --version\n", program);
    printf("  %s probe <unit>\n", program);
    printf("  %s read-sector <unit> <cylinder> <head> <sector>\n", program);
    printf("  %s adf-info <path>\n", program);
    printf("  %s adf-read-sector <path> <cylinder> <head> <sector>\n", program);
    printf("  %s image-adf <unit> <path>\n", program);
    printf("  %s verify-adf <unit> <path>\n", program);
    printf("  %s restore-preflight <path> <unit> <ERASE-DFn>\n", program);
    printf("  %s restore-adf <path> <unit> <ERASE-DFn>\n", program);
    printf("  %s copy-disk <source-unit> <destination-unit> <ERASE-DFn>\n", program);
    printf("  %s recover-read <unit> <cylinder> <head> <sector> <attempts>\n", program);
    printf("  %s recover-image <unit> <image-path> <map-path> <attempts>\n", program);
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

static int command_verify_adf(ULONG unit, const char *path)
{
    AdVerifyReport report;
    AdVerifyResult result;

    printf("Verifying DF%u: against %s\n", (unsigned int)unit, path);
    result = ad_verify_disk_against_adf(unit, path, &report);
    if (result == AD_VERIFY_OK) {
        printf("verify-adf OK: identical sectors=%u change=%u\n",
               (unsigned int)report.sectors_compared,
               (unsigned int)report.end_change_number);
        return 0;
    }

    if (result == AD_VERIFY_MISMATCH) {
        fprintf(stderr,
                "verify-adf data mismatch: sectors=%u first=C%u H%u S%u byte=%u offset=%u disk=%02x adf=%02x\n",
                (unsigned int)report.mismatch_sectors,
                (unsigned int)report.first_cylinder,
                (unsigned int)report.first_head,
                (unsigned int)report.first_sector,
                (unsigned int)report.first_byte_in_sector,
                (unsigned int)report.first_absolute_offset,
                (unsigned int)report.first_disk_byte,
                (unsigned int)report.first_adf_byte);
        return 3;
    }

    fprintf(stderr, "verify-adf failed: %s", ad_verify_result_string(result));
    if (report.source_result != AD_TD_OK) {
        fprintf(stderr, " (%s)", ad_td_result_string(report.source_result));
    } else if (report.adf_result != AD_ADF_OK) {
        fprintf(stderr, " (%s)", ad_adf_result_string(report.adf_result));
    }
    fputc('\n', stderr);
    return 2;
}

static int command_restore_preflight(const char *path, ULONG unit,
                                     const char *confirmation)
{
    AdRestorePreflightReport report;
    AdRestorePreflightResult result;

    printf("Restore preflight: %s -> DF%u\n", path, (unsigned int)unit);
    puts("WARNING: a later restore command will destroy all data on the destination disk.");
    result = ad_restore_preflight(path, unit, confirmation, &report);
    if (result != AD_RESTORE_PREFLIGHT_OK) {
        fprintf(stderr, "restore-preflight failed: %s", ad_restore_preflight_result_string(result));
        if (report.adf_result != AD_ADF_OK) {
            fprintf(stderr, " (%s)", ad_adf_result_string(report.adf_result));
        } else if (report.destination_result != AD_TD_OK) {
            fprintf(stderr, " (%s)", ad_td_result_string(report.destination_result));
        }
        fputc('\n', stderr);
        puts("NO WRITE PERFORMED");
        return 2;
    }

    printf("restore-preflight OK: source=%u bytes media=present write-protected=no change=%u\n",
           (unsigned int)report.source_bytes,
           (unsigned int)report.end_change_number);
    puts("NO WRITE PERFORMED");
    return 0;
}

static int command_restore_adf(const char *path, ULONG unit,
                               const char *confirmation)
{
    AdRestoreReport report;
    AdRestoreResult result;

    printf("RESTORE: %s -> DF%u\n", path, (unsigned int)unit);
    puts("WARNING: this command overwrites the destination disk.");
    result = ad_restore_adf_to_disk(path, unit, confirmation, &report);
    if (result != AD_RESTORE_OK) {
        fprintf(stderr,
                "restore-adf failed: %s after written=%u verified=%u at C%u H%u S%u",
                ad_restore_result_string(result),
                (unsigned int)report.sectors_written,
                (unsigned int)report.sectors_verified,
                (unsigned int)report.failure_cylinder,
                (unsigned int)report.failure_head,
                (unsigned int)report.failure_sector);
        if (report.preflight_result != AD_RESTORE_PREFLIGHT_OK) {
            fprintf(stderr, " (%s)",
                    ad_restore_preflight_result_string(report.preflight_result));
        } else if (report.adf_result != AD_ADF_OK) {
            fprintf(stderr, " (%s)", ad_adf_result_string(report.adf_result));
        } else if (report.destination_result != AD_TD_OK) {
            fprintf(stderr, " (%s)", ad_td_result_string(report.destination_result));
        } else if (result == AD_RESTORE_ERR_VERIFY) {
            fprintf(stderr, " byte=%u expected=%02x actual=%02x",
                    (unsigned int)report.failure_byte,
                    (unsigned int)report.expected_byte,
                    (unsigned int)report.actual_byte);
        }
        fputc('\n', stderr);
        return 2;
    }

    printf("restore-adf OK: sectors-written=%u sectors-verified=%u bytes=%u change=%u\n",
           (unsigned int)report.sectors_written,
           (unsigned int)report.sectors_verified,
           (unsigned int)report.bytes_written,
           (unsigned int)report.end_change_number);
    return 0;
}

static int command_copy_disk(ULONG source_unit, ULONG destination_unit,
                             const char *confirmation)
{
    AdCopyDiskReport report;
    AdCopyDiskResult result;

    printf("COPY: DF%u -> DF%u\n", (unsigned int)source_unit,
           (unsigned int)destination_unit);
    puts("WARNING: this command overwrites the destination disk.");
    result = ad_copy_disk(source_unit, destination_unit, confirmation, &report);
    if (result != AD_COPY_DISK_OK) {
        fprintf(stderr,
                "copy-disk failed: %s after read=%u written=%u verified=%u at C%u H%u S%u",
                ad_copy_disk_result_string(result),
                (unsigned int)report.sectors_read,
                (unsigned int)report.sectors_written,
                (unsigned int)report.sectors_verified,
                (unsigned int)report.failure_cylinder,
                (unsigned int)report.failure_head,
                (unsigned int)report.failure_sector);
        if (report.source_result != AD_TD_OK) {
            fprintf(stderr, " (source: %s)", ad_td_result_string(report.source_result));
        } else if (report.destination_result != AD_TD_OK) {
            fprintf(stderr, " (destination: %s)",
                    ad_td_result_string(report.destination_result));
        } else if (result == AD_COPY_DISK_ERR_VERIFY) {
            fprintf(stderr, " byte=%u expected=%02x actual=%02x",
                    (unsigned int)report.failure_byte,
                    (unsigned int)report.expected_byte,
                    (unsigned int)report.actual_byte);
        }
        fputc('\n', stderr);
        return 2;
    }

    printf("copy-disk OK: sectors-read=%u sectors-written=%u sectors-verified=%u bytes=%u source-change=%u destination-change=%u\n",
           (unsigned int)report.sectors_read,
           (unsigned int)report.sectors_written,
           (unsigned int)report.sectors_verified,
           (unsigned int)report.bytes_written,
           (unsigned int)report.source_end_change_number,
           (unsigned int)report.destination_end_change_number);
    return 0;
}

static int command_recover_read(ULONG unit, ULONG cylinder, ULONG head,
                                ULONG sector, ULONG attempts)
{
    AdRecoverySectorRecord record;
    AdRecoveryReadResult result;
    unsigned char buffer[AD_TD_SECTOR_SIZE];

    result = ad_recovery_read_sector(unit, cylinder, head, sector, attempts,
                                     buffer, &record);
    if (result != AD_RECOVERY_READ_OK) {
        fprintf(stderr,
                "recover-read failed: %s at C%u H%u S%u attempts=%u change=%u->%u",
                ad_recovery_read_result_string(result),
                (unsigned int)record.cylinder,
                (unsigned int)record.head,
                (unsigned int)record.sector,
                (unsigned int)record.attempts,
                (unsigned int)record.start_change_number,
                (unsigned int)record.end_change_number);
        if (record.last_source_result != AD_TD_OK) {
            fprintf(stderr, " (%s)", ad_td_result_string(record.last_source_result));
        }
        fputc('\n', stderr);
        return 2;
    }

    printf("recover-read OK: DF%u C%u H%u S%u attempts=%u change=%u\n",
           (unsigned int)unit,
           (unsigned int)cylinder,
           (unsigned int)head,
           (unsigned int)sector,
           (unsigned int)record.attempts,
           (unsigned int)record.end_change_number);
    print_prefix16(buffer);
    return 0;
}

static int command_recover_image(ULONG unit, const char *image_path,
                                 const char *map_path, ULONG attempts)
{
    AdRecoveryImageReport report;
    AdRecoveryImageResult result;

    printf("Recovery imaging DF%u: -> %s (map %s, attempts=%u)\n",
           (unsigned int)unit, image_path, map_path, (unsigned int)attempts);
    result = ad_recovery_image_disk(unit, image_path, map_path, attempts,
                                    &report);

    if (result == AD_RECOVERY_IMAGE_OK) {
        printf("recover-image OK: good=%u bad=%u unread=%u placeholders=%u sectors=%u bytes=%u change=%u map=%s\n",
               (unsigned int)report.good_sectors,
               (unsigned int)report.bad_sectors,
               (unsigned int)report.unread_sectors,
               (unsigned int)report.placeholder_sectors,
               (unsigned int)report.sectors_output,
               (unsigned int)report.bytes_output,
               (unsigned int)report.end_change_number,
               map_path);
        return 0;
    }

    if (result == AD_RECOVERY_IMAGE_PARTIAL) {
        fprintf(stderr,
                "recover-image PARTIAL: good=%u bad=%u unread=%u placeholders=%u sectors=%u bytes=%u change=%u->%u; consult map %s",
                (unsigned int)report.good_sectors,
                (unsigned int)report.bad_sectors,
                (unsigned int)report.unread_sectors,
                (unsigned int)report.placeholder_sectors,
                (unsigned int)report.sectors_output,
                (unsigned int)report.bytes_output,
                (unsigned int)report.start_change_number,
                (unsigned int)report.end_change_number,
                map_path);
        if (report.first_bad_valid) {
            fprintf(stderr, " first-bad=C%u/H%u/S%u",
                    (unsigned int)report.first_bad_cylinder,
                    (unsigned int)report.first_bad_head,
                    (unsigned int)report.first_bad_sector);
        }
        if (report.first_unread_valid) {
            fprintf(stderr, " first-unread=C%u/H%u/S%u",
                    (unsigned int)report.first_unread_cylinder,
                    (unsigned int)report.first_unread_head,
                    (unsigned int)report.first_unread_sector);
        }
        fputc('\n', stderr);
        return 3;
    }

    fprintf(stderr, "recover-image failed: %s",
            ad_recovery_image_result_string(result));
    if (report.last_source_result != AD_TD_OK) {
        fprintf(stderr, " (%s)", ad_td_result_string(report.last_source_result));
    }
    fputc('\n', stderr);
    return 2;
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
    ULONG source_unit;
    ULONG destination_unit;
    ULONG cylinder;
    ULONG head;
    ULONG sector;
    ULONG attempts;

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

    if (argc == 4 && strcmp(argv[1], "verify-adf") == 0 && parse_ulong(argv[2], &unit)) {
        if (unit > 3UL) {
            fprintf(stderr, "unit must be 0..3\n");
            return 1;
        }
        return command_verify_adf(unit, argv[3]);
    }

    if (argc == 5 && strcmp(argv[1], "restore-preflight") == 0 &&
        parse_ulong(argv[3], &unit)) {
        if (unit > 3UL) {
            fprintf(stderr, "unit must be 0..3\n");
            return 1;
        }
        return command_restore_preflight(argv[2], unit, argv[4]);
    }

    if (argc == 5 && strcmp(argv[1], "restore-adf") == 0 &&
        parse_ulong(argv[3], &unit)) {
        if (unit > 3UL) {
            fprintf(stderr, "unit must be 0..3\n");
            return 1;
        }
        return command_restore_adf(argv[2], unit, argv[4]);
    }

    if (argc == 5 && strcmp(argv[1], "copy-disk") == 0 &&
        parse_ulong(argv[2], &source_unit) &&
        parse_ulong(argv[3], &destination_unit)) {
        if (source_unit > 3UL || destination_unit > 3UL) {
            fprintf(stderr, "source and destination units must be 0..3\n");
            return 1;
        }
        if (source_unit == destination_unit) {
            fprintf(stderr, "source and destination units must differ\n");
            return 1;
        }
        return command_copy_disk(source_unit, destination_unit, argv[4]);
    }

    if (argc == 7 && strcmp(argv[1], "recover-read") == 0 &&
        parse_ulong(argv[2], &unit) && parse_ulong(argv[3], &cylinder) &&
        parse_ulong(argv[4], &head) && parse_ulong(argv[5], &sector) &&
        parse_ulong(argv[6], &attempts)) {
        if (unit > 3UL) {
            fprintf(stderr, "unit must be 0..3\n");
            return 1;
        }
        if (attempts == 0UL || attempts > AD_RECOVERY_MAX_ATTEMPTS) {
            fprintf(stderr, "attempts must be 1..16\n");
            return 1;
        }
        return command_recover_read(unit, cylinder, head, sector, attempts);
    }

    if (argc == 6 && strcmp(argv[1], "recover-image") == 0 &&
        parse_ulong(argv[2], &unit) && parse_ulong(argv[5], &attempts)) {
        if (unit > 3UL) {
            fprintf(stderr, "unit must be 0..3\n");
            return 1;
        }
        if (attempts == 0UL || attempts > AD_RECOVERY_MAX_ATTEMPTS) {
            fprintf(stderr, "attempts must be 1..16\n");
            return 1;
        }
        return command_recover_image(unit, argv[3], argv[4], attempts);
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
