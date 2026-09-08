#ifndef AMIDISK_RECOVERY_READ_H
#define AMIDISK_RECOVERY_READ_H

#include <exec/types.h>

#include "io/trackdisk/trackdisk.h"

#define AD_RECOVERY_MAX_ATTEMPTS 16UL

typedef enum AdRecoveryReadResult {
    AD_RECOVERY_READ_OK = 0,
    AD_RECOVERY_READ_ERR_ARGUMENT = -1,
    AD_RECOVERY_READ_ERR_SOURCE_OPEN = -2,
    AD_RECOVERY_READ_ERR_SOURCE_STATUS = -3,
    AD_RECOVERY_READ_ERR_NO_MEDIA = -4,
    AD_RECOVERY_READ_ERR_MEDIA_CHANGED = -5,
    AD_RECOVERY_READ_ERR_EXHAUSTED = -6
} AdRecoveryReadResult;

typedef struct AdRecoverySectorRecord {
    ULONG cylinder;
    ULONG head;
    ULONG sector;
    ULONG attempts;
    ULONG start_change_number;
    ULONG end_change_number;
    AdTdResult last_source_result;
} AdRecoverySectorRecord;

AdRecoveryReadResult ad_recovery_read_sector(ULONG unit,
                                             ULONG cylinder,
                                             ULONG head,
                                             ULONG sector,
                                             ULONG max_attempts,
                                             void *buffer,
                                             AdRecoverySectorRecord *record);
const char *ad_recovery_read_result_string(AdRecoveryReadResult result);

#endif
