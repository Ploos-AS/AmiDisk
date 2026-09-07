#ifndef AMIDISK_RESTORE_PREFLIGHT_H
#define AMIDISK_RESTORE_PREFLIGHT_H

#include <exec/types.h>

#include "io/adf/adf.h"
#include "io/trackdisk/trackdisk.h"

typedef enum AdRestorePreflightResult {
    AD_RESTORE_PREFLIGHT_OK = 0,
    AD_RESTORE_PREFLIGHT_ERR_ARGUMENT = -1,
    AD_RESTORE_PREFLIGHT_ERR_CONFIRMATION = -2,
    AD_RESTORE_PREFLIGHT_ERR_ADF = -3,
    AD_RESTORE_PREFLIGHT_ERR_DEST_OPEN = -4,
    AD_RESTORE_PREFLIGHT_ERR_DEST_STATUS = -5,
    AD_RESTORE_PREFLIGHT_ERR_NO_MEDIA = -6,
    AD_RESTORE_PREFLIGHT_ERR_WRITE_PROTECTED = -7,
    AD_RESTORE_PREFLIGHT_ERR_DEST_READ = -8,
    AD_RESTORE_PREFLIGHT_ERR_MEDIA_CHANGED = -9
} AdRestorePreflightResult;

typedef struct AdRestorePreflightReport {
    ULONG source_bytes;
    ULONG start_change_number;
    ULONG end_change_number;
    LONG media_present;
    LONG write_protected;
    AdAdfResult adf_result;
    AdTdResult destination_result;
} AdRestorePreflightReport;

AdRestorePreflightResult ad_restore_preflight(const char *path, ULONG unit,
                                               const char *confirmation,
                                               AdRestorePreflightReport *report);
const char *ad_restore_preflight_result_string(AdRestorePreflightResult result);

#endif
