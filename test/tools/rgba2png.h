#ifndef RGBA_OSD_TEST_TOOLS_RGBA_TO_PNG_H
#define RGBA_OSD_TEST_TOOLS_RGBA_TO_PNG_H

#include "str.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see https://github.com/miloyip/svpng/blob/master/svpng.inc
 */
void rgba_2_png(vfs_str_t* fp, unsigned w, unsigned h, const unsigned char* img, int alpha);

#ifdef __cplusplus
}
#endif
#endif
