#ifndef AMIDISK_COPY_DISK_H
#define AMIDISK_COPY_DISK_H

#include <exec/types.h>

#include "io/trackdisk/trackdisk.h"

typedef enum AdCopyDiskResult {
    AD_COPY_DISK_OK = 0,
    AD_COPY_DISK_ERR_ARGUMENT = -1,
    AD_COPY_DISK_ERR_CONFIRMATION = -2,
    AD_COPY_DISK_ERR_SOURCE_OPEN = -3,
    AD_COPY_DISK_ERR_SOURCE_STATUS = -4,
    AD_COPY_DISK_ERR_DEST_OPEN = -5,
    AD_COPY_DISK_ERR_DEST_STATUS = -6,
    AD_COPY_DISK_ERR_SOURCE_CHANGED = -7,
    AD_COPY_DISK_ERR_DEST_CHANGED = -8,
    AD_COPY_DISK_ERR_SOURCE_READ = -9,
    AD_COPY_DISK_ERR_DEST_WRITE = -10,
    AD_COPY_DISK_ERR_DEST_READBACK = -11,
    AD_COPY_DISK_ERR_VERIFY = -12
} AdCopyDiskResult;

typedef struct AdCopyDiskReport {
    ULONG sectors_read;
    ULONG sectors_written;
    ULONG sectors_verified;
    ULONG bytes_written;
    ULONG source_start_change_number;
    ULONG source_end_change_number;
    ULONG destination_start_change_number;
    ULONG destination_end_change_number;
    ULONG failure_cylinder;
    ULONG failure_head;
    ULONG failure_sector;
    ULONG failure_byte;
    unsigned char expected_byte;
    unsigned char actual_byte;
    AdTdResult source_result;
    AdTdResult destination_result;
} AdCopyDiskReport;

AdCopyDiskResult ad_copy_disk(ULONG source_unit, ULONG destination_unit,
                              const char *confirmation,
                              AdCopyDiskReport *report);
const char *ad_copy_disk_result_string(AdCopyDiskResult result);

#endif
