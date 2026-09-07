#include "operations/verify_adf.h"

#include <string.h>

static ULONG ad_verify_absolute_offset(ULONG cylinder, ULONG head,
                                       ULONG sector, ULONG byte_in_sector)
{
    ULONG logical_sector;

    logical_sector = ((cylinder * AD_TD_HEADS) + head) *
                     AD_TD_SECTORS_PER_TRACK + sector;
    return logical_sector * AD_TD_SECTOR_SIZE + byte_in_sector;
}

AdVerifyResult ad_verify_disk_against_adf(ULONG unit, const char *path,
                                         AdVerifyReport *report)
{
    AdTrackDisk disk;
    AdTdStatus status;
    AdTdResult td_result;
    AdAdfImage image;
    AdAdfResult adf_result;
    unsigned char disk_buffer[AD_TD_SECTOR_SIZE];
    unsigned char adf_buffer[AD_ADF_SECTOR_SIZE];
    ULONG cylinder;
    ULONG head;
    ULONG sector;
    ULONG byte_index;
    ULONG start_change;
    ULONG end_change;
    ULONG mismatch_sectors = 0UL;

    if (report != NULL) {
        memset(report, 0, sizeof(*report));
        report->source_result = AD_TD_OK;
        report->adf_result = AD_ADF_OK;
    }

    if (unit > 3UL || path == NULL || *path == '\0') {
        return AD_VERIFY_ERR_ARGUMENT;
    }

    td_result = ad_td_open(&disk, unit);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
        }
        return AD_VERIFY_ERR_SOURCE_OPEN;
    }

    td_result = ad_td_get_status(&disk, &status);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
        }
        ad_td_close(&disk);
        return AD_VERIFY_ERR_SOURCE_STATUS;
    }
    if (!status.media_present) {
        ad_td_close(&disk);
        return AD_VERIFY_ERR_NO_MEDIA;
    }

    td_result = ad_td_get_change_number(&disk, &start_change);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
        }
        ad_td_close(&disk);
        return AD_VERIFY_ERR_SOURCE_STATUS;
    }
    if (report != NULL) {
        report->start_change_number = start_change;
    }

    adf_result = ad_adf_open(&image, path);
    if (adf_result != AD_ADF_OK) {
        if (report != NULL) {
            report->adf_result = adf_result;
        }
        ad_td_close(&disk);
        return AD_VERIFY_ERR_ADF_OPEN;
    }

    for (cylinder = 0UL; cylinder < AD_TD_CYLINDERS; ++cylinder) {
        for (head = 0UL; head < AD_TD_HEADS; ++head) {
            for (sector = 0UL; sector < AD_TD_SECTORS_PER_TRACK; ++sector) {
                td_result = ad_td_read_sector(&disk, cylinder, head, sector,
                                              disk_buffer);
                if (td_result != AD_TD_OK) {
                    if (report != NULL) {
                        report->source_result = td_result;
                    }
                    ad_adf_close(&image);
                    ad_td_close(&disk);
                    return AD_VERIFY_ERR_SOURCE_READ;
                }

                adf_result = ad_adf_read_sector(&image, cylinder, head, sector,
                                                adf_buffer);
                if (adf_result != AD_ADF_OK) {
                    if (report != NULL) {
                        report->adf_result = adf_result;
                    }
                    ad_adf_close(&image);
                    ad_td_close(&disk);
                    return AD_VERIFY_ERR_ADF_READ;
                }

                if (report != NULL) {
                    ++report->sectors_compared;
                }

                if (memcmp(disk_buffer, adf_buffer,
                           (size_t)AD_TD_SECTOR_SIZE) != 0) {
                    if (mismatch_sectors == 0UL && report != NULL) {
                        for (byte_index = 0UL;
                             byte_index < AD_TD_SECTOR_SIZE;
                             ++byte_index) {
                            if (disk_buffer[byte_index] != adf_buffer[byte_index]) {
                                report->first_cylinder = cylinder;
                                report->first_head = head;
                                report->first_sector = sector;
                                report->first_byte_in_sector = byte_index;
                                report->first_absolute_offset =
                                    ad_verify_absolute_offset(cylinder, head,
                                                              sector, byte_index);
                                report->first_disk_byte = disk_buffer[byte_index];
                                report->first_adf_byte = adf_buffer[byte_index];
                                break;
                            }
                        }
                    }
                    ++mismatch_sectors;
                    if (report != NULL) {
                        report->mismatch_sectors = mismatch_sectors;
                    }
                }
            }
        }
    }

    td_result = ad_td_get_change_number(&disk, &end_change);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
        }
        ad_adf_close(&image);
        ad_td_close(&disk);
        return AD_VERIFY_ERR_SOURCE_STATUS;
    }
    if (report != NULL) {
        report->end_change_number = end_change;
    }

    ad_adf_close(&image);
    ad_td_close(&disk);

    if (end_change != start_change) {
        return AD_VERIFY_ERR_MEDIA_CHANGED;
    }

    if (mismatch_sectors != 0UL) {
        return AD_VERIFY_MISMATCH;
    }

    return AD_VERIFY_OK;
}

const char *ad_verify_result_string(AdVerifyResult result)
{
    switch (result) {
    case AD_VERIFY_OK:
        return "identical";
    case AD_VERIFY_MISMATCH:
        return "data mismatch";
    case AD_VERIFY_ERR_ARGUMENT:
        return "invalid argument";
    case AD_VERIFY_ERR_SOURCE_OPEN:
        return "cannot open source drive";
    case AD_VERIFY_ERR_SOURCE_STATUS:
        return "cannot query source drive";
    case AD_VERIFY_ERR_NO_MEDIA:
        return "no media present";
    case AD_VERIFY_ERR_ADF_OPEN:
        return "cannot open reference ADF";
    case AD_VERIFY_ERR_SOURCE_READ:
        return "source read failed";
    case AD_VERIFY_ERR_ADF_READ:
        return "ADF read failed";
    case AD_VERIFY_ERR_MEDIA_CHANGED:
        return "source media changed during verify";
    default:
        return "unknown verify error";
    }
}
