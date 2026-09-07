#ifndef AMIDISK_VERIFY_ADF_H
#define AMIDISK_VERIFY_ADF_H

#include <exec/types.h>

#include "io/adf/adf.h"
#include "io/trackdisk/trackdisk.h"

typedef enum AdVerifyResult {
    AD_VERIFY_OK = 0,
    AD_VERIFY_MISMATCH = 1,
    AD_VERIFY_ERR_ARGUMENT = -1,
    AD_VERIFY_ERR_SOURCE_OPEN = -2,
    AD_VERIFY_ERR_SOURCE_STATUS = -3,
    AD_VERIFY_ERR_NO_MEDIA = -4,
    AD_VERIFY_ERR_ADF_OPEN = -5,
    AD_VERIFY_ERR_SOURCE_READ = -6,
    AD_VERIFY_ERR_ADF_READ = -7,
    AD_VERIFY_ERR_MEDIA_CHANGED = -8
} AdVerifyResult;

typedef struct AdVerifyReport {
    ULONG sectors_compared;
    ULONG mismatch_sectors;
    ULONG first_cylinder;
    ULONG first_head;
    ULONG first_sector;
    ULONG first_byte_in_sector;
    ULONG first_absolute_offset;
    UBYTE first_disk_byte;
    UBYTE first_adf_byte;
    ULONG start_change_number;
    ULONG end_change_number;
    AdTdResult source_result;
    AdAdfResult adf_result;
} AdVerifyReport;

AdVerifyResult ad_verify_disk_against_adf(ULONG unit, const char *path,
                                         AdVerifyReport *report);
const char *ad_verify_result_string(AdVerifyResult result);

#endif
