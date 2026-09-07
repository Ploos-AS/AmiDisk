#include "operations/image_adf.h"

#include <stdio.h>
#include <string.h>

#include "io/adf/adf.h"

static int ad_image_destination_exists(const char *path)
{
    FILE *file;

    file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }

    fclose(file);
    return 1;
}

static void ad_image_discard_partial(FILE *output, const char *path)
{
    if (output != NULL) {
        fclose(output);
    }
    remove(path);
}

AdImageResult ad_image_disk_to_adf(ULONG unit, const char *path,
                                   AdImageReport *report)
{
    AdTrackDisk disk;
    AdTdStatus status;
    AdTdResult td_result;
    FILE *output;
    unsigned char buffer[AD_TD_SECTOR_SIZE];
    ULONG cylinder;
    ULONG head;
    ULONG sector;
    ULONG start_change;
    ULONG end_change;
    size_t written;

    if (report != NULL) {
        memset(report, 0, sizeof(*report));
        report->source_result = AD_TD_OK;
    }

    if (unit > 3UL || path == NULL || *path == '\0') {
        return AD_IMAGE_ERR_ARGUMENT;
    }

    if (ad_image_destination_exists(path)) {
        return AD_IMAGE_ERR_DEST_EXISTS;
    }

    td_result = ad_td_open(&disk, unit);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
        }
        return AD_IMAGE_ERR_SOURCE_OPEN;
    }

    td_result = ad_td_get_status(&disk, &status);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
        }
        ad_td_close(&disk);
        return AD_IMAGE_ERR_SOURCE_STATUS;
    }
    if (!status.media_present) {
        ad_td_close(&disk);
        return AD_IMAGE_ERR_NO_MEDIA;
    }

    td_result = ad_td_get_change_number(&disk, &start_change);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
        }
        ad_td_close(&disk);
        return AD_IMAGE_ERR_SOURCE_STATUS;
    }
    if (report != NULL) {
        report->start_change_number = start_change;
    }

    output = fopen(path, "wb");
    if (output == NULL) {
        ad_td_close(&disk);
        return AD_IMAGE_ERR_DEST_OPEN;
    }

    for (cylinder = 0UL; cylinder < AD_TD_CYLINDERS; ++cylinder) {
        for (head = 0UL; head < AD_TD_HEADS; ++head) {
            for (sector = 0UL; sector < AD_TD_SECTORS_PER_TRACK; ++sector) {
                td_result = ad_td_read_sector(&disk, cylinder, head, sector, buffer);
                if (td_result != AD_TD_OK) {
                    if (report != NULL) {
                        report->source_result = td_result;
                    }
                    ad_image_discard_partial(output, path);
                    ad_td_close(&disk);
                    return AD_IMAGE_ERR_SOURCE_READ;
                }

                written = fwrite(buffer, 1U, (size_t)AD_TD_SECTOR_SIZE, output);
                if (written != (size_t)AD_TD_SECTOR_SIZE) {
                    ad_image_discard_partial(output, path);
                    ad_td_close(&disk);
                    return AD_IMAGE_ERR_DEST_WRITE;
                }

                if (report != NULL) {
                    ++report->sectors_written;
                    report->bytes_written += AD_TD_SECTOR_SIZE;
                }
            }
        }
    }

    td_result = ad_td_get_change_number(&disk, &end_change);
    if (td_result != AD_TD_OK) {
        if (report != NULL) {
            report->source_result = td_result;
        }
        ad_image_discard_partial(output, path);
        ad_td_close(&disk);
        return AD_IMAGE_ERR_SOURCE_STATUS;
    }
    if (report != NULL) {
        report->end_change_number = end_change;
    }

    if (end_change != start_change) {
        ad_image_discard_partial(output, path);
        ad_td_close(&disk);
        return AD_IMAGE_ERR_MEDIA_CHANGED;
    }

    if (fclose(output) != 0) {
        remove(path);
        ad_td_close(&disk);
        return AD_IMAGE_ERR_DEST_WRITE;
    }

    ad_td_close(&disk);
    return AD_IMAGE_OK;
}

const char *ad_image_result_string(AdImageResult result)
{
    switch (result) {
    case AD_IMAGE_OK:
        return "ok";
    case AD_IMAGE_ERR_ARGUMENT:
        return "invalid argument";
    case AD_IMAGE_ERR_DEST_EXISTS:
        return "destination already exists";
    case AD_IMAGE_ERR_DEST_OPEN:
        return "cannot create destination ADF";
    case AD_IMAGE_ERR_SOURCE_OPEN:
        return "cannot open source drive";
    case AD_IMAGE_ERR_SOURCE_STATUS:
        return "cannot query source drive";
    case AD_IMAGE_ERR_NO_MEDIA:
        return "no media present";
    case AD_IMAGE_ERR_SOURCE_READ:
        return "source read failed";
    case AD_IMAGE_ERR_DEST_WRITE:
        return "destination write failed";
    case AD_IMAGE_ERR_MEDIA_CHANGED:
        return "source media changed during imaging";
    default:
        return "unknown imaging error";
    }
}
