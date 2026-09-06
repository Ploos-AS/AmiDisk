#ifndef AMIDISK_ADF_H
#define AMIDISK_ADF_H

#include <exec/types.h>
#include <stdio.h>

#define AD_ADF_SECTOR_SIZE 512UL
#define AD_ADF_SECTORS_PER_TRACK 11UL
#define AD_ADF_HEADS 2UL
#define AD_ADF_CYLINDERS 80UL
#define AD_ADF_SECTOR_COUNT (AD_ADF_SECTORS_PER_TRACK * AD_ADF_HEADS * AD_ADF_CYLINDERS)
#define AD_ADF_DISK_BYTES (AD_ADF_SECTOR_SIZE * AD_ADF_SECTOR_COUNT)

typedef enum AdAdfResult {
    AD_ADF_OK = 0,
    AD_ADF_ERR_ARGUMENT = -1,
    AD_ADF_ERR_OPEN = -2,
    AD_ADF_ERR_SIZE = -3,
    AD_ADF_ERR_RANGE = -4,
    AD_ADF_ERR_SEEK = -5,
    AD_ADF_ERR_READ = -6
} AdAdfResult;

typedef struct AdAdfImage {
    FILE *file;
    ULONG size_bytes;
} AdAdfImage;

AdAdfResult ad_adf_open(AdAdfImage *image, const char *path);
void ad_adf_close(AdAdfImage *image);
AdAdfResult ad_adf_read_sector(AdAdfImage *image, ULONG cylinder, ULONG head,
                               ULONG sector, void *buffer);
const char *ad_adf_result_string(AdAdfResult result);

#endif
