#include "operations/copy_disk.h"

#include <stdio.h>
#include <string.h>

static void ad_copy_close_pair(AdTrackDisk *source, AdTrackDisk *destination)
{
    ad_td_close(destination);
    ad_td_close(source);
}

static AdCopyDiskResult ad_copy_check_changes(AdTrackDisk *source,
                                               AdTrackDisk *destination,
                                               ULONG source_start,
                                               ULONG destination_start,
                                               ULONG cylinder,
                                               ULONG head,
                                               ULONG sector,
                                               AdCopyDiskReport *report)
{
    AdTdResult td_result;
    ULONG change_number;

    td_result = ad_td_get_change_number(source, &change_number);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
            report->failure_cylinder = cylinder;
            report->failure_head = head;
            report->failure_sector = sector;
        }
        return AD_COPY_DISK_ERR_SOURCE_STATUS;
    }
    if (report != NULL) {
        report->source_end_change_number = change_number;
    }
    if (change_number != source_start) {
        if (report != NULL) {
            report->failure_cylinder = cylinder;
            report->failure_head = head;
            report->failure_sector = sector;
        }
        return AD_COPY_DISK_ERR_SOURCE_CHANGED;
    }

    td_result = ad_td_get_change_number(destination, &change_number);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->destination_result = td_result;
            report->failure_cylinder = cylinder;
            report->failure_head = head;
            report->failure_sector = sector;
        }
        return AD_COPY_DISK_ERR_DEST_STATUS;
    }
    if (report != NULL) {
        report->destination_end_change_number = change_number;
    }
    if (change_number != destination_start) {
        if (report != NULL) {
            report->failure_cylinder = cylinder;
            report->failure_head = head;
            report->failure_sector = sector;
        }
        return AD_COPY_DISK_ERR_DEST_CHANGED;
    }

    return AD_COPY_DISK_OK;
}

AdCopyDiskResult ad_copy_disk(ULONG source_unit, ULONG destination_unit,
                              const char *confirmation,
                              AdCopyDiskReport *report)
{
    AdTrackDisk source;
    AdTrackDisk destination;
    AdTdStatus source_status;
    AdTdStatus destination_status;
    AdTdResult td_result;
    AdCopyDiskResult copy_result;
    unsigned char source_buffer[AD_TD_SECTOR_SIZE];
    unsigned char readback_buffer[AD_TD_SECTOR_SIZE];
    char expected_confirmation[10];
    ULONG source_start;
    ULONG destination_start;
    ULONG cylinder;
    ULONG head;
    ULONG sector;
    ULONG byte_index;

    if (report != NULL) {
        memset(report, 0, sizeof(*report));
        report->source_result = AD_TD_OK;
        report->destination_result = AD_TD_OK;
    }

    if (source_unit > 3UL || destination_unit > 3UL ||
        source_unit == destination_unit || confirmation == NULL) {
        return AD_COPY_DISK_ERR_ARGUMENT;
    }

    sprintf(expected_confirmation, "ERASE-DF%u", (unsigned int)destination_unit);
    if (strcmp(confirmation, expected_confirmation) != 0) {
        return AD_COPY_DISK_ERR_CONFIRMATION;
    }

    td_result = ad_td_open(&source, source_unit);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
        }
        return AD_COPY_DISK_ERR_SOURCE_OPEN;
    }

    td_result = ad_td_get_status(&source, &source_status);
    if (td_result != AD_TD_OK || !source_status.media_present) {
        if (report != NULL) {
            report->source_result = td_result != AD_TD_OK ? td_result : AD_TD_ERR_NO_MEDIA;
        }
        ad_td_close(&source);
        return AD_COPY_DISK_ERR_SOURCE_STATUS;
    }

    td_result = ad_td_open(&destination, destination_unit);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->destination_result = td_result;
        }
        ad_td_close(&source);
        return AD_COPY_DISK_ERR_DEST_OPEN;
    }

    td_result = ad_td_get_status(&destination, &destination_status);
    if (td_result != AD_TD_OK || !destination_status.media_present ||
        destination_status.write_protected) {
        if (report != NULL) {
            report->destination_result = td_result != AD_TD_OK ? td_result :
                (destination_status.media_present ? AD_TD_ERR_WRITE_PROTECTED :
                                                    AD_TD_ERR_NO_MEDIA);
        }
        ad_copy_close_pair(&source, &destination);
        return AD_COPY_DISK_ERR_DEST_STATUS;
    }

    td_result = ad_td_get_change_number(&source, &source_start);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
        }
        ad_copy_close_pair(&source, &destination);
        return AD_COPY_DISK_ERR_SOURCE_STATUS;
    }

    td_result = ad_td_get_change_number(&destination, &destination_start);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->destination_result = td_result;
        }
        ad_copy_close_pair(&source, &destination);
        return AD_COPY_DISK_ERR_DEST_STATUS;
    }

    if (report != NULL) {
        report->source_start_change_number = source_start;
        report->source_end_change_number = source_start;
        report->destination_start_change_number = destination_start;
        report->destination_end_change_number = destination_start;
    }

    td_result = ad_td_read_sector(&source, 0UL, 0UL, 0UL, source_buffer);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
        }
        ad_copy_close_pair(&source, &destination);
        return AD_COPY_DISK_ERR_SOURCE_READ;
    }

    td_result = ad_td_read_sector(&destination, 0UL, 0UL, 0UL, readback_buffer);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->destination_result = td_result;
        }
        ad_copy_close_pair(&source, &destination);
        return AD_COPY_DISK_ERR_DEST_READBACK;
    }

    copy_result = ad_copy_check_changes(&source, &destination,
                                        source_start, destination_start,
                                        0UL, 0UL, 0UL, report);
    if (copy_result != AD_COPY_DISK_OK) {
        ad_copy_close_pair(&source, &destination);
        return copy_result;
    }

    for (cylinder = 0UL; cylinder < AD_TD_CYLINDERS; ++cylinder) {
        for (head = 0UL; head < AD_TD_HEADS; ++head) {
            for (sector = 0UL; sector < AD_TD_SECTORS_PER_TRACK; ++sector) {
                copy_result = ad_copy_check_changes(&source, &destination,
                                                    source_start, destination_start,
                                                    cylinder, head, sector, report);
                if (copy_result != AD_COPY_DISK_OK) {
                    ad_copy_close_pair(&source, &destination);
                    return copy_result;
                }

                td_result = ad_td_read_sector(&source, cylinder, head, sector,
                                              source_buffer);
                if (td_result != AD_TD_OK) {
                    if (report != NULL) {
                        report->source_result = td_result;
                        report->failure_cylinder = cylinder;
                        report->failure_head = head;
                        report->failure_sector = sector;
                    }
                    ad_copy_close_pair(&source, &destination);
                    return AD_COPY_DISK_ERR_SOURCE_READ;
                }
                if (report != NULL) {
                    ++report->sectors_read;
                }

                copy_result = ad_copy_check_changes(&source, &destination,
                                                    source_start, destination_start,
                                                    cylinder, head, sector, report);
                if (copy_result != AD_COPY_DISK_OK) {
                    ad_copy_close_pair(&source, &destination);
                    return copy_result;
                }

                td_result = ad_td_write_sector(&destination, cylinder, head, sector,
                                               source_buffer);
                if (td_result != AD_TD_OK) {
                    if (report != NULL) {
                        report->destination_result = td_result;
                        report->failure_cylinder = cylinder;
                        report->failure_head = head;
                        report->failure_sector = sector;
                    }
                    ad_copy_close_pair(&source, &destination);
                    return AD_COPY_DISK_ERR_DEST_WRITE;
                }
                if (report != NULL) {
                    ++report->sectors_written;
                    report->bytes_written += AD_TD_SECTOR_SIZE;
                }

                td_result = ad_td_read_sector(&destination, cylinder, head, sector,
                                              readback_buffer);
                if (td_result != AD_TD_OK) {
                    if (report != NULL) {
                        report->destination_result = td_result;
                        report->failure_cylinder = cylinder;
                        report->failure_head = head;
                        report->failure_sector = sector;
                    }
                    ad_copy_close_pair(&source, &destination);
                    return AD_COPY_DISK_ERR_DEST_READBACK;
                }

                if (memcmp(source_buffer, readback_buffer,
                           (size_t)AD_TD_SECTOR_SIZE) != 0) {
                    if (report != NULL) {
                        report->failure_cylinder = cylinder;
                        report->failure_head = head;
                        report->failure_sector = sector;
                        for (byte_index = 0UL; byte_index < AD_TD_SECTOR_SIZE;
                             ++byte_index) {
                            if (source_buffer[byte_index] != readback_buffer[byte_index]) {
                                report->failure_byte = byte_index;
                                report->expected_byte = source_buffer[byte_index];
                                report->actual_byte = readback_buffer[byte_index];
                                break;
                            }
                        }
                    }
                    ad_copy_close_pair(&source, &destination);
                    return AD_COPY_DISK_ERR_VERIFY;
                }
                if (report != NULL) {
                    ++report->sectors_verified;
                }

                copy_result = ad_copy_check_changes(&source, &destination,
                                                    source_start, destination_start,
                                                    cylinder, head, sector, report);
                if (copy_result != AD_COPY_DISK_OK) {
                    ad_copy_close_pair(&source, &destination);
                    return copy_result;
                }
            }
        }
    }

    ad_copy_close_pair(&source, &destination);
    return AD_COPY_DISK_OK;
}

const char *ad_copy_disk_result_string(AdCopyDiskResult result)
{
    switch (result) {
    case AD_COPY_DISK_OK:
        return "disk copy complete and verified";
    case AD_COPY_DISK_ERR_ARGUMENT:
        return "invalid argument or source equals destination";
    case AD_COPY_DISK_ERR_CONFIRMATION:
        return "destination confirmation does not match";
    case AD_COPY_DISK_ERR_SOURCE_OPEN:
        return "cannot open source drive";
    case AD_COPY_DISK_ERR_SOURCE_STATUS:
        return "source status check failed";
    case AD_COPY_DISK_ERR_DEST_OPEN:
        return "cannot open destination drive";
    case AD_COPY_DISK_ERR_DEST_STATUS:
        return "destination status check failed";
    case AD_COPY_DISK_ERR_SOURCE_CHANGED:
        return "source media changed during copy";
    case AD_COPY_DISK_ERR_DEST_CHANGED:
        return "destination media changed during copy";
    case AD_COPY_DISK_ERR_SOURCE_READ:
        return "source read failed";
    case AD_COPY_DISK_ERR_DEST_WRITE:
        return "destination write failed";
    case AD_COPY_DISK_ERR_DEST_READBACK:
        return "destination readback failed";
    case AD_COPY_DISK_ERR_VERIFY:
        return "destination readback verification failed";
    default:
        return "unknown disk copy error";
    }
}
