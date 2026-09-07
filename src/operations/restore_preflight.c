#include "operations/restore_preflight.h"

#include <stdio.h>
#include <string.h>

static int ad_restore_confirmation_matches(ULONG unit, const char *confirmation)
{
    char expected[16];

    if (confirmation == NULL) {
        return 0;
    }

    sprintf(expected, "ERASE-DF%u", (unsigned int)unit);
    return strcmp(confirmation, expected) == 0;
}

AdRestorePreflightResult ad_restore_preflight(const char *path, ULONG unit,
                                               const char *confirmation,
                                               AdRestorePreflightReport *report)
{
    AdAdfImage image;
    AdAdfResult adf_result;
    AdTrackDisk disk;
    AdTdStatus status;
    AdTdResult td_result;
    unsigned char sector0[AD_TD_SECTOR_SIZE];
    ULONG start_change;
    ULONG end_change;

    if (report != NULL) {
        memset(report, 0, sizeof(*report));
        report->adf_result = AD_ADF_OK;
        report->destination_result = AD_TD_OK;
    }

    if (path == NULL || *path == '\0' || unit > 3UL) {
        return AD_RESTORE_PREFLIGHT_ERR_ARGUMENT;
    }

    if (!ad_restore_confirmation_matches(unit, confirmation)) {
        return AD_RESTORE_PREFLIGHT_ERR_CONFIRMATION;
    }

    adf_result = ad_adf_open(&image, path);
    if (adf_result != AD_ADF_OK) {
        if (report != NULL) {
            report->adf_result = adf_result;
        }
        return AD_RESTORE_PREFLIGHT_ERR_ADF;
    }
    if (report != NULL) {
        report->source_bytes = image.size_bytes;
    }

    td_result = ad_td_open(&disk, unit);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->destination_result = td_result;
        }
        ad_adf_close(&image);
        return AD_RESTORE_PREFLIGHT_ERR_DEST_OPEN;
    }

    td_result = ad_td_get_status(&disk, &status);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->destination_result = td_result;
        }
        ad_td_close(&disk);
        ad_adf_close(&image);
        return AD_RESTORE_PREFLIGHT_ERR_DEST_STATUS;
    }

    if (report != NULL) {
        report->media_present = status.media_present;
        report->write_protected = status.write_protected;
    }

    if (!status.media_present) {
        ad_td_close(&disk);
        ad_adf_close(&image);
        return AD_RESTORE_PREFLIGHT_ERR_NO_MEDIA;
    }
    if (status.write_protected) {
        ad_td_close(&disk);
        ad_adf_close(&image);
        return AD_RESTORE_PREFLIGHT_ERR_WRITE_PROTECTED;
    }

    td_result = ad_td_get_change_number(&disk, &start_change);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->destination_result = td_result;
        }
        ad_td_close(&disk);
        ad_adf_close(&image);
        return AD_RESTORE_PREFLIGHT_ERR_DEST_STATUS;
    }
    if (report != NULL) {
        report->start_change_number = start_change;
    }

    td_result = ad_td_read_sector(&disk, 0UL, 0UL, 0UL, sector0);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->destination_result = td_result;
        }
        ad_td_close(&disk);
        ad_adf_close(&image);
        return AD_RESTORE_PREFLIGHT_ERR_DEST_READ;
    }

    td_result = ad_td_get_change_number(&disk, &end_change);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->destination_result = td_result;
        }
        ad_td_close(&disk);
        ad_adf_close(&image);
        return AD_RESTORE_PREFLIGHT_ERR_DEST_STATUS;
    }
    if (report != NULL) {
        report->end_change_number = end_change;
    }

    ad_td_close(&disk);
    ad_adf_close(&image);

    if (end_change != start_change) {
        return AD_RESTORE_PREFLIGHT_ERR_MEDIA_CHANGED;
    }

    return AD_RESTORE_PREFLIGHT_OK;
}

const char *ad_restore_preflight_result_string(AdRestorePreflightResult result)
{
    switch (result) {
    case AD_RESTORE_PREFLIGHT_OK:
        return "preflight passed; no write performed";
    case AD_RESTORE_PREFLIGHT_ERR_ARGUMENT:
        return "invalid argument";
    case AD_RESTORE_PREFLIGHT_ERR_CONFIRMATION:
        return "confirmation rejected";
    case AD_RESTORE_PREFLIGHT_ERR_ADF:
        return "source ADF rejected";
    case AD_RESTORE_PREFLIGHT_ERR_DEST_OPEN:
        return "cannot open destination drive";
    case AD_RESTORE_PREFLIGHT_ERR_DEST_STATUS:
        return "cannot query destination drive";
    case AD_RESTORE_PREFLIGHT_ERR_NO_MEDIA:
        return "no destination media present";
    case AD_RESTORE_PREFLIGHT_ERR_WRITE_PROTECTED:
        return "destination media is write-protected";
    case AD_RESTORE_PREFLIGHT_ERR_DEST_READ:
        return "destination media read test failed";
    case AD_RESTORE_PREFLIGHT_ERR_MEDIA_CHANGED:
        return "destination media changed during preflight";
    default:
        return "unknown restore preflight error";
    }
}
