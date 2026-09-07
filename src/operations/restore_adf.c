#include "operations/restore_adf.h"

#include <string.h>

AdRestoreResult ad_restore_adf_to_disk(const char *path, ULONG unit,
                                       const char *confirmation,
                                       AdRestoreReport *report)
{
    AdRestorePreflightReport preflight_report;
    AdRestorePreflightResult preflight_result;
    AdAdfImage image;
    AdAdfResult adf_result;
    AdTrackDisk disk;
    AdTdStatus status;
    AdTdResult td_result;
    unsigned char source_buffer[AD_ADF_SECTOR_SIZE];
    unsigned char readback_buffer[AD_TD_SECTOR_SIZE];
    ULONG cylinder;
    ULONG head;
    ULONG sector;
    ULONG byte_index;
    ULONG change_number;
    ULONG start_change;

    if (report != NULL) {
        memset(report, 0, sizeof(*report));
        report->preflight_result = AD_RESTORE_PREFLIGHT_OK;
        report->adf_result = AD_ADF_OK;
        report->destination_result = AD_TD_OK;
    }

    if (path == NULL || *path == '\0' || unit > 3UL || confirmation == NULL) {
        return AD_RESTORE_ERR_ARGUMENT;
    }

    preflight_result = ad_restore_preflight(path, unit, confirmation,
                                            &preflight_report);
    if (report != NULL) {
        report->preflight_result = preflight_result;
    }
    if (preflight_result != AD_RESTORE_PREFLIGHT_OK) {
        return AD_RESTORE_ERR_PREFLIGHT;
    }

    adf_result = ad_adf_open(&image, path);
    if (adf_result != AD_ADF_OK) {
        if (report != NULL) {
            report->adf_result = adf_result;
        }
        return AD_RESTORE_ERR_ADF_OPEN;
    }

    td_result = ad_td_open(&disk, unit);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->destination_result = td_result;
        }
        ad_adf_close(&image);
        return AD_RESTORE_ERR_DEST_OPEN;
    }

    td_result = ad_td_get_status(&disk, &status);
    if (td_result != AD_TD_OK || !status.media_present || status.write_protected) {
        if (report != NULL) {
            report->destination_result = td_result != AD_TD_OK ? td_result :
                (status.media_present ? AD_TD_ERR_WRITE_PROTECTED : AD_TD_ERR_NO_MEDIA);
        }
        ad_td_close(&disk);
        ad_adf_close(&image);
        return AD_RESTORE_ERR_DEST_STATUS;
    }

    td_result = ad_td_get_change_number(&disk, &start_change);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->destination_result = td_result;
        }
        ad_td_close(&disk);
        ad_adf_close(&image);
        return AD_RESTORE_ERR_DEST_STATUS;
    }
    if (report != NULL) {
        report->start_change_number = start_change;
    }

    if (start_change != preflight_report.end_change_number) {
        ad_td_close(&disk);
        ad_adf_close(&image);
        return AD_RESTORE_ERR_MEDIA_CHANGED;
    }

    for (cylinder = 0UL; cylinder < AD_TD_CYLINDERS; ++cylinder) {
        for (head = 0UL; head < AD_TD_HEADS; ++head) {
            for (sector = 0UL; sector < AD_TD_SECTORS_PER_TRACK; ++sector) {
                td_result = ad_td_get_change_number(&disk, &change_number);
                if (td_result != AD_TD_OK) {
                    if (report != NULL) {
                        report->destination_result = td_result;
                        report->failure_cylinder = cylinder;
                        report->failure_head = head;
                        report->failure_sector = sector;
                    }
                    ad_td_close(&disk);
                    ad_adf_close(&image);
                    return AD_RESTORE_ERR_DEST_STATUS;
                }
                if (change_number != start_change) {
                    if (report != NULL) {
                        report->end_change_number = change_number;
                        report->failure_cylinder = cylinder;
                        report->failure_head = head;
                        report->failure_sector = sector;
                    }
                    ad_td_close(&disk);
                    ad_adf_close(&image);
                    return AD_RESTORE_ERR_MEDIA_CHANGED;
                }

                adf_result = ad_adf_read_sector(&image, cylinder, head, sector,
                                                source_buffer);
                if (adf_result != AD_ADF_OK) {
                    if (report != NULL) {
                        report->adf_result = adf_result;
                        report->failure_cylinder = cylinder;
                        report->failure_head = head;
                        report->failure_sector = sector;
                    }
                    ad_td_close(&disk);
                    ad_adf_close(&image);
                    return AD_RESTORE_ERR_ADF_READ;
                }

                td_result = ad_td_write_sector(&disk, cylinder, head, sector,
                                               source_buffer);
                if (td_result != AD_TD_OK) {
                    if (report != NULL) {
                        report->destination_result = td_result;
                        report->failure_cylinder = cylinder;
                        report->failure_head = head;
                        report->failure_sector = sector;
                    }
                    ad_td_close(&disk);
                    ad_adf_close(&image);
                    return AD_RESTORE_ERR_DEST_WRITE;
                }
                if (report != NULL) {
                    ++report->sectors_written;
                    report->bytes_written += AD_TD_SECTOR_SIZE;
                }

                td_result = ad_td_read_sector(&disk, cylinder, head, sector,
                                              readback_buffer);
                if (td_result != AD_TD_OK) {
                    if (report != NULL) {
                        report->destination_result = td_result;
                        report->failure_cylinder = cylinder;
                        report->failure_head = head;
                        report->failure_sector = sector;
                    }
                    ad_td_close(&disk);
                    ad_adf_close(&image);
                    return AD_RESTORE_ERR_DEST_READBACK;
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
                    ad_td_close(&disk);
                    ad_adf_close(&image);
                    return AD_RESTORE_ERR_VERIFY;
                }
                if (report != NULL) {
                    ++report->sectors_verified;
                }

                td_result = ad_td_get_change_number(&disk, &change_number);
                if (td_result != AD_TD_OK) {
                    if (report != NULL) {
                        report->destination_result = td_result;
                        report->failure_cylinder = cylinder;
                        report->failure_head = head;
                        report->failure_sector = sector;
                    }
                    ad_td_close(&disk);
                    ad_adf_close(&image);
                    return AD_RESTORE_ERR_DEST_STATUS;
                }
                if (change_number != start_change) {
                    if (report != NULL) {
                        report->end_change_number = change_number;
                        report->failure_cylinder = cylinder;
                        report->failure_head = head;
                        report->failure_sector = sector;
                    }
                    ad_td_close(&disk);
                    ad_adf_close(&image);
                    return AD_RESTORE_ERR_MEDIA_CHANGED;
                }
            }
        }
    }

    if (report != NULL) {
        report->end_change_number = start_change;
    }
    ad_td_close(&disk);
    ad_adf_close(&image);
    return AD_RESTORE_OK;
}

const char *ad_restore_result_string(AdRestoreResult result)
{
    switch (result) {
    case AD_RESTORE_OK:
        return "restore complete and verified";
    case AD_RESTORE_ERR_ARGUMENT:
        return "invalid argument";
    case AD_RESTORE_ERR_PREFLIGHT:
        return "restore preflight failed";
    case AD_RESTORE_ERR_ADF_OPEN:
        return "cannot open source ADF";
    case AD_RESTORE_ERR_DEST_OPEN:
        return "cannot open destination drive";
    case AD_RESTORE_ERR_DEST_STATUS:
        return "destination status check failed";
    case AD_RESTORE_ERR_MEDIA_CHANGED:
        return "destination media changed during restore";
    case AD_RESTORE_ERR_ADF_READ:
        return "source ADF read failed";
    case AD_RESTORE_ERR_DEST_WRITE:
        return "destination write failed";
    case AD_RESTORE_ERR_DEST_READBACK:
        return "destination readback failed";
    case AD_RESTORE_ERR_VERIFY:
        return "destination readback verification failed";
    default:
        return "unknown restore error";
    }
}
