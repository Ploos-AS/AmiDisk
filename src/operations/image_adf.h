#ifndef AMIDISK_IMAGE_ADF_H
#define AMIDISK_IMAGE_ADF_H

#include <exec/types.h>

#include "io/trackdisk/trackdisk.h"

typedef enum AdImageResult {
    AD_IMAGE_OK = 0,
    AD_IMAGE_ERR_ARGUMENT = -1,
    AD_IMAGE_ERR_DEST_EXISTS = -2,
    AD_IMAGE_ERR_DEST_OPEN = -3,
    AD_IMAGE_ERR_SOURCE_OPEN = -4,
    AD_IMAGE_ERR_SOURCE_STATUS = -5,
    AD_IMAGE_ERR_NO_MEDIA = -6,
    AD_IMAGE_ERR_SOURCE_READ = -7,
    AD_IMAGE_ERR_DEST_WRITE = -8,
    AD_IMAGE_ERR_MEDIA_CHANGED = -9
} AdImageResult;

typedef struct AdImageReport {
    ULONG sectors_written;
    ULONG bytes_written;
    ULONG start_change_number;
    ULONG end_change_number;
    AdTdResult source_result;
} AdImageReport;

AdImageResult ad_image_disk_to_adf(ULONG unit, const char *path,
                                   AdImageReport *report);
const char *ad_image_result_string(AdImageResult result);

#endif
