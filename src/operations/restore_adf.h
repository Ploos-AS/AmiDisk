#ifndef AMIDISK_RESTORE_ADF_H
#define AMIDISK_RESTORE_ADF_H

#include <exec/types.h>

#include "io/adf/adf.h"
#include "io/trackdisk/trackdisk.h"
#include "operations/restore_preflight.h"

typedef enum AdRestoreResult {
    AD_RESTORE_OK = 0,
    AD_RESTORE_ERR_ARGUMENT = -1,
    AD_RESTORE_ERR_PREFLIGHT = -2,
    AD_RESTORE_ERR_ADF_OPEN = -3,
    AD_RESTORE_ERR_DEST_OPEN = -4,
    AD_RESTORE_ERR_DEST_STATUS = -5,
    AD_RESTORE_ERR_MEDIA_CHANGED = -6,
    AD_RESTORE_ERR_ADF_READ = -7,
    AD_RESTORE_ERR_DEST_WRITE = -8,
    AD_RESTORE_ERR_DEST_READBACK = -9,
    AD_RESTORE_ERR_VERIFY = -10
} AdRestoreResult;

typedef struct AdRestoreReport {
    ULONG sectors_written;
    ULONG sectors_verified;
    ULONG bytes_written;
    ULONG start_change_number;
    ULONG end_change_number;
    ULONG failure_cylinder;
    ULONG failure_head;
    ULONG failure_sector;
    ULONG failure_byte;
    unsigned char expected_byte;
    unsigned char actual_byte;
    AdRestorePreflightResult preflight_result;
    AdAdfResult adf_result;
    AdTdResult destination_result;
} AdRestoreReport;

AdRestoreResult ad_restore_adf_to_disk(const char *path, ULONG unit,
                                       const char *confirmation,
                                       AdRestoreReport *report);
const char *ad_restore_result_string(AdRestoreResult result);

#endif
