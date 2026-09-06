#include <exec/io.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>

#include "io/trackdisk/trackdisk.h"

static AdTdResult ad_td_do(AdTrackDisk *disk, UWORD command, APTR data,
                           ULONG length, ULONG offset)
{
    LONG rc;

    if (disk == NULL || disk->io == NULL || !disk->opened) {
        return AD_TD_ERR_ARGUMENT;
    }

    disk->io->iotd_Req.io_Command = command;
    disk->io->iotd_Req.io_Data = data;
    disk->io->iotd_Req.io_Length = length;
    disk->io->iotd_Req.io_Offset = offset;
    rc = DoIO((struct IORequest *)disk->io);
    if (rc != 0 || disk->io->iotd_Req.io_Error != 0) {
        return AD_TD_ERR_IO;
    }

    return AD_TD_OK;
}

AdTdResult ad_td_open(AdTrackDisk *disk, ULONG unit)
{
    LONG rc;

    if (disk == NULL || unit > 3UL) {
        return AD_TD_ERR_ARGUMENT;
    }

    memset(disk, 0, sizeof(*disk));
    disk->unit = unit;

    disk->port = CreateMsgPort();
    if (disk->port == NULL) {
        return AD_TD_ERR_PORT;
    }

    disk->io = (struct IOExtTD *)CreateIORequest(disk->port, sizeof(struct IOExtTD));
    if (disk->io == NULL) {
        DeleteMsgPort(disk->port);
        disk->port = NULL;
        return AD_TD_ERR_REQUEST;
    }

    rc = OpenDevice("trackdisk.device", unit, (struct IORequest *)disk->io, 0);
    if (rc != 0) {
        DeleteIORequest((struct IORequest *)disk->io);
        DeleteMsgPort(disk->port);
        disk->io = NULL;
        disk->port = NULL;
        return AD_TD_ERR_OPEN;
    }

    disk->opened = 1;
    return AD_TD_OK;
}

void ad_td_close(AdTrackDisk *disk)
{
    if (disk == NULL) {
        return;
    }

    if (disk->opened && disk->io != NULL) {
        CloseDevice((struct IORequest *)disk->io);
    }
    if (disk->io != NULL) {
        DeleteIORequest((struct IORequest *)disk->io);
    }
    if (disk->port != NULL) {
        DeleteMsgPort(disk->port);
    }

    memset(disk, 0, sizeof(*disk));
}

AdTdResult ad_td_get_status(AdTrackDisk *disk, AdTdStatus *status)
{
    AdTdResult result;

    if (status == NULL) {
        return AD_TD_ERR_ARGUMENT;
    }

    result = ad_td_do(disk, TD_CHANGESTATE, NULL, 0, 0);
    if (result != AD_TD_OK) {
        return result;
    }
    status->media_present = (disk->io->iotd_Req.io_Actual == 0);

    result = ad_td_do(disk, TD_PROTSTATUS, NULL, 0, 0);
    if (result != AD_TD_OK) {
        return result;
    }
    status->write_protected = (disk->io->iotd_Req.io_Actual != 0);

    return AD_TD_OK;
}

AdTdResult ad_td_read_sector(AdTrackDisk *disk, ULONG cylinder, ULONG head,
                             ULONG sector, void *buffer)
{
    AdTdStatus status;
    ULONG logical_sector;
    ULONG offset;
    AdTdResult result;

    if (buffer == NULL) {
        return AD_TD_ERR_ARGUMENT;
    }
    if (cylinder >= AD_TD_CYLINDERS || head >= AD_TD_HEADS ||
        sector >= AD_TD_SECTORS_PER_TRACK) {
        return AD_TD_ERR_RANGE;
    }

    result = ad_td_get_status(disk, &status);
    if (result != AD_TD_OK) {
        return result;
    }
    if (!status.media_present) {
        return AD_TD_ERR_NO_MEDIA;
    }

    logical_sector = ((cylinder * AD_TD_HEADS) + head) * AD_TD_SECTORS_PER_TRACK + sector;
    offset = logical_sector * AD_TD_SECTOR_SIZE;

    return ad_td_do(disk, CMD_READ, buffer, AD_TD_SECTOR_SIZE, offset);
}

const char *ad_td_result_string(AdTdResult result)
{
    switch (result) {
    case AD_TD_OK: return "ok";
    case AD_TD_ERR_ARGUMENT: return "invalid argument";
    case AD_TD_ERR_PORT: return "could not create message port";
    case AD_TD_ERR_REQUEST: return "could not create I/O request";
    case AD_TD_ERR_OPEN: return "could not open trackdisk.device unit";
    case AD_TD_ERR_IO: return "trackdisk.device I/O error";
    case AD_TD_ERR_NO_MEDIA: return "no media present";
    case AD_TD_ERR_RANGE: return "sector address out of range";
    default: return "unknown error";
    }
}
