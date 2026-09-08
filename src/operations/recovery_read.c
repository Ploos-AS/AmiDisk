#include "operations/recovery_read.h"

#include <string.h>

static void ad_recovery_record_init(AdRecoverySectorRecord *record,
                                    ULONG cylinder,
                                    ULONG head,
                                    ULONG sector)
{
    if (record != NULL) {
        memset(record, 0, sizeof(*record));
        record->cylinder = cylinder;
        record->head = head;
        record->sector = sector;
        record->last_source_result = AD_TD_OK;
    }
}

AdRecoveryReadResult ad_recovery_read_sector(ULONG unit,
                                             ULONG cylinder,
                                             ULONG head,
                                             ULONG sector,
                                             ULONG max_attempts,
                                             void *buffer,
                                             AdRecoverySectorRecord *record)
{
    AdTrackDisk disk;
    AdTdStatus status;
    AdTdResult td_result;
    ULONG start_change;
    ULONG end_change;
    ULONG attempt;

    ad_recovery_record_init(record, cylinder, head, sector);

    if (unit > 3UL || cylinder >= AD_TD_CYLINDERS ||
        head >= AD_TD_HEADS || sector >= AD_TD_SECTORS_PER_TRACK ||
        max_attempts == 0UL || max_attempts > AD_RECOVERY_MAX_ATTEMPTS ||
        buffer == NULL) {
        return AD_RECOVERY_READ_ERR_ARGUMENT;
    }

    td_result = ad_td_open(&disk, unit);
    if (td_result != AD_TD_OK) {
        if (record != NULL) {
            record->last_source_result = td_result;
        }
        return AD_RECOVERY_READ_ERR_SOURCE_OPEN;
    }

    td_result = ad_td_get_status(&disk, &status);
    if (td_result != AD_TD_OK) {
        if (record != NULL) {
            record->last_source_result = td_result;
        }
        ad_td_close(&disk);
        return AD_RECOVERY_READ_ERR_SOURCE_STATUS;
    }
    if (!status.media_present) {
        if (record != NULL) {
            record->last_source_result = AD_TD_ERR_NO_MEDIA;
        }
        ad_td_close(&disk);
        return AD_RECOVERY_READ_ERR_NO_MEDIA;
    }

    td_result = ad_td_get_change_number(&disk, &start_change);
    if (td_result != AD_TD_OK) {
        if (record != NULL) {
            record->last_source_result = td_result;
        }
        ad_td_close(&disk);
        return AD_RECOVERY_READ_ERR_SOURCE_STATUS;
    }
    if (record != NULL) {
        record->start_change_number = start_change;
        record->end_change_number = start_change;
    }

    for (attempt = 1UL; attempt <= max_attempts; ++attempt) {
        td_result = ad_td_get_change_number(&disk, &end_change);
        if (td_result != AD_TD_OK) {
            if (record != NULL) {
                record->attempts = attempt - 1UL;
                record->last_source_result = td_result;
            }
            ad_td_close(&disk);
            return AD_RECOVERY_READ_ERR_SOURCE_STATUS;
        }
        if (record != NULL) {
            record->end_change_number = end_change;
        }
        if (end_change != start_change) {
            ad_td_close(&disk);
            return AD_RECOVERY_READ_ERR_MEDIA_CHANGED;
        }

        td_result = ad_td_read_sector(&disk, cylinder, head, sector, buffer);
        if (record != NULL) {
            record->attempts = attempt;
            record->last_source_result = td_result;
        }

        if (td_result == AD_TD_OK) {
            td_result = ad_td_get_change_number(&disk, &end_change);
            if (td_result != AD_TD_OK) {
                if (record != NULL) {
                    record->last_source_result = td_result;
                }
                ad_td_close(&disk);
                return AD_RECOVERY_READ_ERR_SOURCE_STATUS;
            }
            if (record != NULL) {
                record->end_change_number = end_change;
            }
            ad_td_close(&disk);
            if (end_change != start_change) {
                return AD_RECOVERY_READ_ERR_MEDIA_CHANGED;
            }
            return AD_RECOVERY_READ_OK;
        }
    }

    td_result = ad_td_get_change_number(&disk, &end_change);
    if (td_result != AD_TD_OK) {
        if (record != NULL) {
            record->last_source_result = td_result;
        }
        ad_td_close(&disk);
        return AD_RECOVERY_READ_ERR_SOURCE_STATUS;
    }
    if (record != NULL) {
        record->end_change_number = end_change;
    }
    ad_td_close(&disk);

    if (end_change != start_change) {
        return AD_RECOVERY_READ_ERR_MEDIA_CHANGED;
    }
    return AD_RECOVERY_READ_ERR_EXHAUSTED;
}

const char *ad_recovery_read_result_string(AdRecoveryReadResult result)
{
    switch (result) {
    case AD_RECOVERY_READ_OK:
        return "sector recovered";
    case AD_RECOVERY_READ_ERR_ARGUMENT:
        return "invalid recovery read argument";
    case AD_RECOVERY_READ_ERR_SOURCE_OPEN:
        return "cannot open source drive";
    case AD_RECOVERY_READ_ERR_SOURCE_STATUS:
        return "cannot query source drive";
    case AD_RECOVERY_READ_ERR_NO_MEDIA:
        return "no media present";
    case AD_RECOVERY_READ_ERR_MEDIA_CHANGED:
        return "source media changed during recovery read";
    case AD_RECOVERY_READ_ERR_EXHAUSTED:
        return "sector unreadable after retry budget";
    default:
        return "unknown recovery read error";
    }
}
