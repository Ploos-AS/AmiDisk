#include <stdio.h>

#include "core/ad_version.h"

int main(void)
{
    puts(ad_version_string());
    puts(AMIDISK_TARGET);
    puts("M0 foundation build - no disk writes are implemented.");
    return 0;
}
