#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "rgba-osd.h"

typedef void *(*rgba_osd_realloc_fn)(void *ptr, size_t size);

struct rgba_osd
{
    rgba_osd_config_t config;      /**< User configuration. */
    size_t            pixel_sz;    /**< The number of pixels. */
    size_t            pixel_bytes; /**< The bytes of pixels. Equal to pixel_sz * sizeof(rgba_osd_pixel_t). */
    rgba_osd_pixel_t *old_pixels;
    rgba_osd_pixel_t *new_pixels;
};

int rgba_osd_init(rgba_osd_t **osd, const rgba_osd_config_t *config)
{
    rgba_osd_realloc_fn realloc_fn = config->realloc != NULL ? config->realloc : realloc;
    size_t              canvas_pixels_cnt = config->size.x * config->size.y;
    size_t              canvas_pixel_bytes = sizeof(rgba_osd_pixel_t) * canvas_pixels_cnt;
    size_t              malloc_sz = sizeof(rgba_osd_t) + canvas_pixel_bytes * 2;
    rgba_osd_t         *handle = realloc_fn(NULL, malloc_sz);
    if (handle == NULL)
    {
        return -ENOMEM;
    }

    memcpy(&handle->config, config, sizeof(*config));
    handle->config.realloc = realloc_fn;
    handle->pixel_sz = canvas_pixels_cnt;
    handle->pixel_bytes = canvas_pixel_bytes;
    handle->old_pixels = (rgba_osd_pixel_t *)(handle + 1);
    handle->new_pixels = handle->old_pixels + canvas_pixels_cnt;

    memset(handle->old_pixels, 0, handle->pixel_bytes);
    memset(handle->new_pixels, 0, handle->pixel_bytes);

    *osd = handle;
    return 0;
}

void rgba_osd_exit(rgba_osd_t *osd)
{
    osd->config.realloc(osd, 0);
}

rgba_osd_pixel_t *rgba_osd_pixels(rgba_osd_t *osd)
{
    return osd->new_pixels;
}

rgba_osd_size_t *rgba_osd_canvas_size(rgba_osd_t *osd)
{
    return &osd->config.size;
}

void rgba_osd_new_frame(rgba_osd_t *osd)
{
    rgba_osd_pixel_t *temp = osd->old_pixels;
    osd->old_pixels = osd->new_pixels;
    osd->new_pixels = temp;
    memset(osd->new_pixels, 0, osd->pixel_bytes);
}

void rgba_osd_end_frame(rgba_osd_t *osd)
{
    (void)osd;
}

void rgba_osd_overlay(rgba_osd_t *osd, const rgba_osd_pixel_t *payload, const rgba_osd_size_t *payload_sz,
                      const rgba_osd_size_t *canvas_pos, rgba_osd_overlay_algorithm_fn algo)
{
    /* Do nothing if position is out of canvas. */
    if (canvas_pos->x > osd->config.size.x || canvas_pos->y > osd->config.size.y)
    {
        return;
    }

    /* We only render available area. */
    rgba_osd_size_t render_size = {
        payload_sz->x + canvas_pos->x > osd->config.size.x ? osd->config.size.x - canvas_pos->x : payload_sz->x,
        payload_sz->y + canvas_pos->y > osd->config.size.y ? osd->config.size.y - canvas_pos->y : payload_sz->y,
    };

    /* Render. */
    rgba_osd_size_t canvas_drawing_pos;
    rgba_osd_size_t payload_pos = { 0, 0 };
    for (payload_pos.y = 0; payload_pos.y < render_size.y; payload_pos.y++)
    {
        for (payload_pos.x = 0; payload_pos.x < render_size.x; payload_pos.x++)
        {
            canvas_drawing_pos.x = payload_pos.x + canvas_pos->x;
            canvas_drawing_pos.y = payload_pos.y + canvas_pos->y;

            size_t canvas_offset = canvas_drawing_pos.y * osd->config.size.x + canvas_drawing_pos.x;
            osd->new_pixels[canvas_offset] =
                algo(osd->new_pixels, &osd->config.size, &canvas_drawing_pos, payload, payload_sz, &payload_pos);
        }
    }
}

rgba_osd_pixel_t rgba_osd_overlay_algorithm_replace_all(
    const rgba_osd_pixel_t *canvas, const rgba_osd_size_t *canvas_sz, const rgba_osd_size_t *canvas_pos,
    const rgba_osd_pixel_t *payload, const rgba_osd_size_t *payload_sz, const rgba_osd_size_t *payload_pos)
{
    (void)canvas;
    (void)canvas_sz;
    (void)canvas_pos;
    return payload[payload_pos->y * payload_sz->x + payload_pos->x];
}

rgba_osd_pixel_t rgba_osd_overlay_algorithm_replace_non_alpha(
    const rgba_osd_pixel_t *canvas, const rgba_osd_size_t *canvas_sz, const rgba_osd_size_t *canvas_pos,
    const rgba_osd_pixel_t *payload, const rgba_osd_size_t *payload_sz, const rgba_osd_size_t *payload_pos)
{
    rgba_osd_pixel_t data = payload[payload_pos->y * payload_sz->x + payload_pos->x];
    if (RGBA_R_A(data) != 0)
    {
        return data;
    }

    return canvas[canvas_pos->y * canvas_sz->x + canvas_pos->x];
}
