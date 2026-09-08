#include "operations/recovery_image.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int ad_recovery_path_exists(const char *path)
{
    FILE *file;

    file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }

    fclose(file);
    return 1;
}

static FILE *ad_recovery_open_exclusive(const char *path, const char *mode)
{
    int fd;
    FILE *file;

    fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0) {
        return NULL;
    }

    file = fdopen(fd, mode);
    if (file == NULL) {
        close(fd);
    }
    return file;
}

static void ad_recovery_remove_outputs(FILE *image, FILE *map,
                                       const char *image_path,
                                       const char *map_path)
{
    if (image != NULL) {
        fclose(image);
    }
    if (map != NULL) {
        fclose(map);
    }
    if (image_path != NULL) {
        remove(image_path);
    }
    if (map_path != NULL) {
        remove(map_path);
    }
}

static void ad_recovery_index_to_chs(ULONG index, ULONG *cylinder,
                                     ULONG *head, ULONG *sector)
{
    ULONG track;

    track = index / AD_TD_SECTORS_PER_TRACK;
    *sector = index % AD_TD_SECTORS_PER_TRACK;
    *cylinder = track / AD_TD_HEADS;
    *head = track % AD_TD_HEADS;
}

const char *ad_recovery_sector_state_string(AdRecoverySectorState state)
{
    switch (state) {
    case AD_RECOVERY_SECTOR_GOOD:
        return "GOOD";
    case AD_RECOVERY_SECTOR_BAD:
        return "BAD";
    case AD_RECOVERY_SECTOR_UNREAD:
        return "UNREAD";
    default:
        return "UNKNOWN";
    }
}

static int ad_recovery_write_map_header(FILE *map, ULONG unit,
                                        ULONG max_attempts)
{
    if (fprintf(map, "AMIDISK_RECOVERY_MAP\t1\n") < 0 ||
        fprintf(map, "geometry\t%u\t%u\t%u\t%u\t%u\t%u\n",
                (unsigned int)AD_TD_CYLINDERS,
                (unsigned int)AD_TD_HEADS,
                (unsigned int)AD_TD_SECTORS_PER_TRACK,
                (unsigned int)AD_TD_SECTOR_SIZE,
                (unsigned int)AD_RECOVERY_TOTAL_SECTORS,
                (unsigned int)AD_TD_DISK_BYTES) < 0 ||
        fprintf(map, "source_unit\t%u\n", (unsigned int)unit) < 0 ||
        fprintf(map, "retry_budget\t%u\n", (unsigned int)max_attempts) < 0 ||
        fprintf(map, "placeholder_byte\t00\n") < 0 ||
        fprintf(map,
                "columns\tindex\tcylinder\thead\tsector\tstate\tattempts\ttd_result\n") < 0) {
        return 0;
    }
    return 1;
}

static int ad_recovery_write_map_row(FILE *map, ULONG index,
                                     ULONG cylinder, ULONG head, ULONG sector,
                                     AdRecoverySectorState state,
                                     ULONG attempts, AdTdResult td_result)
{
    return fprintf(map, "%u\t%u\t%u\t%u\t%s\t%u\t%d\n",
                   (unsigned int)index,
                   (unsigned int)cylinder,
                   (unsigned int)head,
                   (unsigned int)sector,
                   ad_recovery_sector_state_string(state),
                   (unsigned int)attempts,
                   (int)td_result) >= 0;
}

static int ad_recovery_write_sector(FILE *image, const unsigned char *buffer)
{
    return fwrite(buffer, 1U, (size_t)AD_TD_SECTOR_SIZE, image) ==
           (size_t)AD_TD_SECTOR_SIZE;
}

static void ad_recovery_report_bad(AdRecoveryImageReport *report,
                                   ULONG cylinder, ULONG head, ULONG sector)
{
    if (report == NULL) {
        return;
    }

    ++report->bad_sectors;
    ++report->placeholder_sectors;
    if (!report->first_bad_valid) {
        report->first_bad_valid = 1UL;
        report->first_bad_cylinder = cylinder;
        report->first_bad_head = head;
        report->first_bad_sector = sector;
    }
}

static void ad_recovery_report_unread(AdRecoveryImageReport *report,
                                      ULONG cylinder, ULONG head, ULONG sector)
{
    if (report == NULL) {
        return;
    }

    ++report->unread_sectors;
    ++report->placeholder_sectors;
    if (!report->first_unread_valid) {
        report->first_unread_valid = 1UL;
        report->first_unread_cylinder = cylinder;
        report->first_unread_head = head;
        report->first_unread_sector = sector;
    }
}

static AdRecoveryImageResult ad_recovery_fill_unread(FILE *image, FILE *map,
                                                      ULONG start_index,
                                                      const unsigned char *zero,
                                                      AdTdResult source_result,
                                                      AdRecoveryImageReport *report)
{
    ULONG index;
    ULONG cylinder;
    ULONG head;
    ULONG sector;

    for (index = start_index; index < AD_RECOVERY_TOTAL_SECTORS; ++index) {
        ad_recovery_index_to_chs(index, &cylinder, &head, &sector);
        if (!ad_recovery_write_sector(image, zero)) {
            return AD_RECOVERY_IMAGE_ERR_DEST_WRITE;
        }
        if (!ad_recovery_write_map_row(map, index, cylinder, head, sector,
                                       AD_RECOVERY_SECTOR_UNREAD, 0UL,
                                       source_result)) {
            return AD_RECOVERY_IMAGE_ERR_MAP_WRITE;
        }
        ad_recovery_report_unread(report, cylinder, head, sector);
        if (report != NULL) {
            ++report->sectors_output;
            report->bytes_output += AD_TD_SECTOR_SIZE;
        }
    }

    return AD_RECOVERY_IMAGE_PARTIAL;
}

AdRecoveryImageResult ad_recovery_image_disk(ULONG unit,
                                             const char *image_path,
                                             const char *map_path,
                                             ULONG max_attempts,
                                             AdRecoveryImageReport *report)
{
    AdTrackDisk monitor;
    AdTdStatus status;
    AdTdResult td_result;
    AdRecoveryReadResult recovery_result;
    AdRecoverySectorRecord record;
    FILE *image;
    FILE *map;
    unsigned char buffer[AD_TD_SECTOR_SIZE];
    unsigned char zero[AD_TD_SECTOR_SIZE];
    ULONG start_change;
    ULONG current_change;
    ULONG index;
    ULONG cylinder;
    ULONG head;
    ULONG sector;
    AdRecoveryImageResult fill_result;
    int partial;

    if (report != NULL) {
        memset(report, 0, sizeof(*report));
        report->last_recovery_result = AD_RECOVERY_READ_OK;
        report->last_source_result = AD_TD_OK;
    }
    memset(zero, 0, sizeof(zero));

    if (unit > 3UL || image_path == NULL || *image_path == '\0' ||
        map_path == NULL || *map_path == '\0' ||
        strcmp(image_path, map_path) == 0 || max_attempts == 0UL ||
        max_attempts > AD_RECOVERY_MAX_ATTEMPTS) {
        return AD_RECOVERY_IMAGE_ERR_ARGUMENT;
    }

    if (ad_recovery_path_exists(image_path) || ad_recovery_path_exists(map_path)) {
        return AD_RECOVERY_IMAGE_ERR_DEST_EXISTS;
    }

    td_result = ad_td_open(&monitor, unit);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->last_source_result = td_result;
        }
        return AD_RECOVERY_IMAGE_ERR_SOURCE_OPEN;
    }

    td_result = ad_td_get_status(&monitor, &status);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->last_source_result = td_result;
        }
        ad_td_close(&monitor);
        return AD_RECOVERY_IMAGE_ERR_SOURCE_STATUS;
    }
    if (!status.media_present) {
        if (report != NULL) {
            report->last_source_result = AD_TD_ERR_NO_MEDIA;
        }
        ad_td_close(&monitor);
        return AD_RECOVERY_IMAGE_ERR_NO_MEDIA;
    }

    td_result = ad_td_get_change_number(&monitor, &start_change);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->last_source_result = td_result;
        }
        ad_td_close(&monitor);
        return AD_RECOVERY_IMAGE_ERR_SOURCE_STATUS;
    }
    if (report != NULL) {
        report->start_change_number = start_change;
        report->end_change_number = start_change;
    }

    image = ad_recovery_open_exclusive(image_path, "wb");
    if (image == NULL) {
        ad_td_close(&monitor);
        return AD_RECOVERY_IMAGE_ERR_DEST_OPEN;
    }

    map = ad_recovery_open_exclusive(map_path, "w");
    if (map == NULL) {
        ad_recovery_remove_outputs(image, NULL, image_path, NULL);
        ad_td_close(&monitor);
        return AD_RECOVERY_IMAGE_ERR_MAP_OPEN;
    }

    if (!ad_recovery_write_map_header(map, unit, max_attempts)) {
        ad_recovery_remove_outputs(image, map, image_path, map_path);
        ad_td_close(&monitor);
        return AD_RECOVERY_IMAGE_ERR_MAP_WRITE;
    }

    partial = 0;

    for (index = 0UL; index < AD_RECOVERY_TOTAL_SECTORS; ++index) {
        ad_recovery_index_to_chs(index, &cylinder, &head, &sector);

        td_result = ad_td_get_change_number(&monitor, &current_change);
        if (td_result != AD_TD_OK || current_change != start_change) {
            if (report != NULL) {
                report->last_source_result = td_result;
                if (td_result == AD_TD_OK) {
                    report->end_change_number = current_change;
                }
            }
            fill_result = ad_recovery_fill_unread(image, map, index, zero,
                                                  td_result, report);
            if (fill_result < 0) {
                ad_recovery_remove_outputs(image, map, image_path, map_path);
                ad_td_close(&monitor);
                return fill_result;
            }
            partial = 1;
            break;
        }

        recovery_result = ad_recovery_read_sector(unit, cylinder, head, sector,
                                                  max_attempts, buffer, &record);
        if (report != NULL) {
            report->last_recovery_result = recovery_result;
            report->last_source_result = record.last_source_result;
        }

        td_result = ad_td_get_change_number(&monitor, &current_change);
        if (td_result != AD_TD_OK || current_change != start_change) {
            if (report != NULL) {
                report->last_source_result = td_result;
                if (td_result == AD_TD_OK) {
                    report->end_change_number = current_change;
                }
            }
            fill_result = ad_recovery_fill_unread(image, map, index, zero,
                                                  td_result, report);
            if (fill_result < 0) {
                ad_recovery_remove_outputs(image, map, image_path, map_path);
                ad_td_close(&monitor);
                return fill_result;
            }
            partial = 1;
            break;
        }

        if (recovery_result == AD_RECOVERY_READ_OK) {
            if (!ad_recovery_write_sector(image, buffer)) {
                ad_recovery_remove_outputs(image, map, image_path, map_path);
                ad_td_close(&monitor);
                return AD_RECOVERY_IMAGE_ERR_DEST_WRITE;
            }
            if (!ad_recovery_write_map_row(map, index, cylinder, head, sector,
                                           AD_RECOVERY_SECTOR_GOOD,
                                           record.attempts,
                                           record.last_source_result)) {
                ad_recovery_remove_outputs(image, map, image_path, map_path);
                ad_td_close(&monitor);
                return AD_RECOVERY_IMAGE_ERR_MAP_WRITE;
            }
            if (report != NULL) {
                ++report->good_sectors;
                ++report->sectors_output;
                report->bytes_output += AD_TD_SECTOR_SIZE;
                report->end_change_number = current_change;
            }
            continue;
        }

        if (recovery_result == AD_RECOVERY_READ_ERR_EXHAUSTED) {
            if (!ad_recovery_write_sector(image, zero)) {
                ad_recovery_remove_outputs(image, map, image_path, map_path);
                ad_td_close(&monitor);
                return AD_RECOVERY_IMAGE_ERR_DEST_WRITE;
            }
            if (!ad_recovery_write_map_row(map, index, cylinder, head, sector,
                                           AD_RECOVERY_SECTOR_BAD,
                                           record.attempts,
                                           record.last_source_result)) {
                ad_recovery_remove_outputs(image, map, image_path, map_path);
                ad_td_close(&monitor);
                return AD_RECOVERY_IMAGE_ERR_MAP_WRITE;
            }
            ad_recovery_report_bad(report, cylinder, head, sector);
            if (report != NULL) {
                ++report->sectors_output;
                report->bytes_output += AD_TD_SECTOR_SIZE;
                report->end_change_number = current_change;
            }
            partial = 1;
            continue;
        }

        fill_result = ad_recovery_fill_unread(image, map, index, zero,
                                              record.last_source_result,
                                              report);
        if (fill_result < 0) {
            ad_recovery_remove_outputs(image, map, image_path, map_path);
            ad_td_close(&monitor);
            return fill_result;
        }
        partial = 1;
        break;
    }

    td_result = ad_td_get_change_number(&monitor, &current_change);
    if (td_result == AD_TD_OK && report != NULL) {
        report->end_change_number = current_change;
    }
    ad_td_close(&monitor);

    if (fclose(image) != 0) {
        fclose(map);
        remove(image_path);
        remove(map_path);
        return AD_RECOVERY_IMAGE_ERR_DEST_WRITE;
    }
    image = NULL;

    if (fclose(map) != 0) {
        remove(image_path);
        remove(map_path);
        return AD_RECOVERY_IMAGE_ERR_MAP_WRITE;
    }

    if (report != NULL && report->sectors_output != AD_RECOVERY_TOTAL_SECTORS) {
        remove(image_path);
        remove(map_path);
        return AD_RECOVERY_IMAGE_ERR_DEST_WRITE;
    }

    if (partial || (report != NULL &&
                    (report->bad_sectors != 0UL || report->unread_sectors != 0UL))) {
        return AD_RECOVERY_IMAGE_PARTIAL;
    }
    return AD_RECOVERY_IMAGE_OK;
}

const char *ad_recovery_image_result_string(AdRecoveryImageResult result)
{
    switch (result) {
    case AD_RECOVERY_IMAGE_OK:
        return "complete recovery image";
    case AD_RECOVERY_IMAGE_PARTIAL:
        return "partial recovery image; consult map";
    case AD_RECOVERY_IMAGE_ERR_ARGUMENT:
        return "invalid recovery image argument";
    case AD_RECOVERY_IMAGE_ERR_DEST_EXISTS:
        return "recovery output already exists";
    case AD_RECOVERY_IMAGE_ERR_DEST_OPEN:
        return "cannot create recovery image";
    case AD_RECOVERY_IMAGE_ERR_MAP_OPEN:
        return "cannot create recovery map";
    case AD_RECOVERY_IMAGE_ERR_SOURCE_OPEN:
        return "cannot open source drive";
    case AD_RECOVERY_IMAGE_ERR_SOURCE_STATUS:
        return "cannot query source drive";
    case AD_RECOVERY_IMAGE_ERR_NO_MEDIA:
        return "no media present";
    case AD_RECOVERY_IMAGE_ERR_DEST_WRITE:
        return "recovery image write failed";
    case AD_RECOVERY_IMAGE_ERR_MAP_WRITE:
        return "recovery map write failed";
    default:
        return "unknown recovery image error";
    }
}
