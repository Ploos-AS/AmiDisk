#include "io/adf/adf.h"

#include <string.h>

static int ad_adf_chs_valid(ULONG cylinder, ULONG head, ULONG sector)
{
    return cylinder < AD_ADF_CYLINDERS &&
           head < AD_ADF_HEADS &&
           sector < AD_ADF_SECTORS_PER_TRACK;
}

static ULONG ad_adf_sector_offset(ULONG cylinder, ULONG head, ULONG sector)
{
    ULONG logical_sector;

    logical_sector = ((cylinder * AD_ADF_HEADS) + head) *
                     AD_ADF_SECTORS_PER_TRACK + sector;
    return logical_sector * AD_ADF_SECTOR_SIZE;
}

AdAdfResult ad_adf_open(AdAdfImage *image, const char *path)
{
    long size;

    if (image == NULL || path == NULL || *path == '\0') {
        return AD_ADF_ERR_ARGUMENT;
    }

    memset(image, 0, sizeof(*image));
    image->file = fopen(path, "rb");
    if (image->file == NULL) {
        return AD_ADF_ERR_OPEN;
    }

    if (fseek(image->file, 0L, SEEK_END) != 0) {
        ad_adf_close(image);
        return AD_ADF_ERR_SEEK;
    }

    size = ftell(image->file);
    if (size < 0L) {
        ad_adf_close(image);
        return AD_ADF_ERR_SEEK;
    }

    if ((ULONG)size != AD_ADF_DISK_BYTES) {
        ad_adf_close(image);
        return AD_ADF_ERR_SIZE;
    }

    if (fseek(image->file, 0L, SEEK_SET) != 0) {
        ad_adf_close(image);
        return AD_ADF_ERR_SEEK;
    }

    image->size_bytes = (ULONG)size;
    return AD_ADF_OK;
}

void ad_adf_close(AdAdfImage *image)
{
    if (image == NULL) {
        return;
    }

    if (image->file != NULL) {
        fclose(image->file);
    }

    image->file = NULL;
    image->size_bytes = 0UL;
}

AdAdfResult ad_adf_read_sector(AdAdfImage *image, ULONG cylinder, ULONG head,
                               ULONG sector, void *buffer)
{
    ULONG offset;
    size_t bytes_read;

    if (image == NULL || image->file == NULL || buffer == NULL) {
        return AD_ADF_ERR_ARGUMENT;
    }

    if (!ad_adf_chs_valid(cylinder, head, sector)) {
        return AD_ADF_ERR_RANGE;
    }

    offset = ad_adf_sector_offset(cylinder, head, sector);
    if (fseek(image->file, (long)offset, SEEK_SET) != 0) {
        return AD_ADF_ERR_SEEK;
    }

    bytes_read = fread(buffer, 1U, (size_t)AD_ADF_SECTOR_SIZE, image->file);
    if (bytes_read != (size_t)AD_ADF_SECTOR_SIZE) {
        return AD_ADF_ERR_READ;
    }

    return AD_ADF_OK;
}

const char *ad_adf_result_string(AdAdfResult result)
{
    switch (result) {
    case AD_ADF_OK:
        return "ok";
    case AD_ADF_ERR_ARGUMENT:
        return "invalid argument";
    case AD_ADF_ERR_OPEN:
        return "cannot open ADF";
    case AD_ADF_ERR_SIZE:
        return "not a standard 880 KiB ADF";
    case AD_ADF_ERR_RANGE:
        return "sector address out of range";
    case AD_ADF_ERR_SEEK:
        return "ADF seek failed";
    case AD_ADF_ERR_READ:
        return "ADF short read or read error";
    default:
        return "unknown ADF error";
    }
}
