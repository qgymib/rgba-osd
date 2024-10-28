#include <string.h>
#include <cutest.h>
#include <rgba-osd.h>
#include "tools/rgba2png.h"

extern unsigned char rectangle_64x32[];
extern unsigned int  rectangle_64x32_len;

static rgba_osd_t *s_osd = NULL;

TEST_FIXTURE_SETUP(compositing)
{
    rgba_osd_config_t config;
    memset(&config, 0, sizeof(config));
    config.size.x = 640;
    config.size.y = 320;

    ASSERT_EQ_INT(rgba_osd_init(&s_osd, &config), 0);
}

TEST_FIXTURE_TEARDOWN(compositing)
{
    rgba_osd_exit(s_osd);
    s_osd = NULL;
}

TEST_F(compositing, over_premultiplied)
{
    rgba_osd_new_frame(s_osd);

    rgba_osd_size_t payload_sz = { 64, 32 };
    rgba_osd_size_t pos = { 0, 0 };
    rgba_osd_overlay(s_osd, (rgba_osd_pixel_t *)rectangle_64x32, &payload_sz, &pos,
                     rgba_osd_overlay_algorithm_compositing_over_premultiplied);

    rgba_osd_end_frame(s_osd);

    vfs_str_t str = VFS_STR_INIT;
    rgba_2_png(&str, 640, 320, (unsigned char *)rgba_osd_canvas(s_osd), 1);

    FILE *f = fopen("out.png", "wb");
    fwrite(str.str, str.len, 1, f);
    fclose(f);
    vfs_str_exit(&str);
}
