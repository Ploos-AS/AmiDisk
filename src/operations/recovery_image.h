#ifndef AMIDISK_RECOVERY_IMAGE_H
#define AMIDISK_RECOVERY_IMAGE_H

#include <exec/types.h>

#include "io/trackdisk/trackdisk.h"
#include "operations/recovery_read.h"

#define AD_RECOVERY_TOTAL_SECTORS \
    (AD_TD_CYLINDERS * AD_TD_HEADS * AD_TD_SECTORS_PER_TRACK)

typedef enum AdRecoverySectorState {
    AD_RECOVERY_SECTOR_GOOD = 0,
    AD_RECOVERY_SECTOR_BAD = 1,
    AD_RECOVERY_SECTOR_UNREAD = 2
} AdRecoverySectorState;

typedef enum AdRecoveryImageResult {
    AD_RECOVERY_IMAGE_OK = 0,
    AD_RECOVERY_IMAGE_PARTIAL = 1,
    AD_RECOVERY_IMAGE_ERR_ARGUMENT = -1,
    AD_RECOVERY_IMAGE_ERR_DEST_EXISTS = -2,
    AD_RECOVERY_IMAGE_ERR_DEST_OPEN = -3,
    AD_RECOVERY_IMAGE_ERR_MAP_OPEN = -4,
    AD_RECOVERY_IMAGE_ERR_SOURCE_OPEN = -5,
    AD_RECOVERY_IMAGE_ERR_SOURCE_STATUS = -6,
    AD_RECOVERY_IMAGE_ERR_NO_MEDIA = -7,
    AD_RECOVERY_IMAGE_ERR_DEST_WRITE = -8,
    AD_RECOVERY_IMAGE_ERR_MAP_WRITE = -9
} AdRecoveryImageResult;

typedef struct AdRecoveryImageReport {
    ULONG good_sectors;
    ULONG bad_sectors;
    ULONG unread_sectors;
    ULONG sectors_output;
    ULONG bytes_output;
    ULONG placeholder_sectors;
    ULONG start_change_number;
    ULONG end_change_number;
    ULONG first_bad_valid;
    ULONG first_bad_cylinder;
    ULONG first_bad_head;
    ULONG first_bad_sector;
    ULONG first_unread_valid;
    ULONG first_unread_cylinder;
    ULONG first_unread_head;
    ULONG first_unread_sector;
    AdRecoveryReadResult last_recovery_result;
    AdTdResult last_source_result;
} AdRecoveryImageReport;

AdRecoveryImageResult ad_recovery_image_disk(ULONG unit,
                                             const char *image_path,
                                             const char *map_path,
                                             ULONG max_attempts,
                                             AdRecoveryImageReport *report);
const char *ad_recovery_image_result_string(AdRecoveryImageResult result);
const char *ad_recovery_sector_state_string(AdRecoverySectorState state);

#endif
