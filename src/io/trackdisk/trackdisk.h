#ifndef AMIDISK_TRACKDISK_H
#define AMIDISK_TRACKDISK_H

#include <exec/types.h>
#include <devices/trackdisk.h>

#define AD_TD_SECTOR_SIZE 512UL
#define AD_TD_SECTORS_PER_TRACK 11UL
#define AD_TD_HEADS 2UL
#define AD_TD_CYLINDERS 80UL
#define AD_TD_DISK_BYTES (AD_TD_SECTOR_SIZE * AD_TD_SECTORS_PER_TRACK * AD_TD_HEADS * AD_TD_CYLINDERS)

typedef enum AdTdResult {
    AD_TD_OK = 0,
    AD_TD_ERR_ARGUMENT = -1,
    AD_TD_ERR_PORT = -2,
    AD_TD_ERR_REQUEST = -3,
    AD_TD_ERR_OPEN = -4,
    AD_TD_ERR_IO = -5,
    AD_TD_ERR_NO_MEDIA = -6,
    AD_TD_ERR_RANGE = -7
} AdTdResult;

typedef struct AdTrackDisk {
    struct MsgPort *port;
    struct IOExtTD *io;
    ULONG unit;
    BYTE opened;
} AdTrackDisk;

typedef struct AdTdStatus {
    LONG media_present;
    LONG write_protected;
} AdTdStatus;

AdTdResult ad_td_open(AdTrackDisk *disk, ULONG unit);
void ad_td_close(AdTrackDisk *disk);
AdTdResult ad_td_get_status(AdTrackDisk *disk, AdTdStatus *status);
AdTdResult ad_td_read_sector(AdTrackDisk *disk, ULONG cylinder, ULONG head,
                             ULONG sector, void *buffer);
const char *ad_td_result_string(AdTdResult result);

#endif
