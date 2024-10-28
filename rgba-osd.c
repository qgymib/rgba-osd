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

/**
 * @brief A simple overlay algoarithm that only consider current pixel.
 */
typedef rgba_osd_pixel_t (*rgba_osd_overlay_compositing_fn)(rgba_osd_pixel_t back, rgba_osd_pixel_t front);

/**
 * @see https://en.wikipedia.org/wiki/Alpha_compositing
 */
static rgba_osd_pixel_t _rgba_osd_overlay_algoarithm_compositing_over_premultiplied(rgba_osd_pixel_t back,
                                                                                    rgba_osd_pixel_t front)
{
    float front_alpha = (float)RGBA_R_A(front) / 255.0f;
    float front_red = (float)RGBA_R_R(front) / 255.0f;
    float front_green = (float)RGBA_R_G(front) / 255.0f;
    float front_blue = (float)RGBA_R_B(front) / 255.0f;
    float back_alpha = (float)RGBA_R_A(back) / 255.0f;
    float back_red = (float)RGBA_R_R(back) / 255.0f;
    float back_green = (float)RGBA_R_G(back) / 255.0f;
    float back_blue = (float)RGBA_R_B(back) / 255.0f;

    float new_alpha = front_alpha + back_alpha * (1 - front_alpha);
    float new_red = front_red + back_red * (1 - front_alpha);
    float new_green = front_green + back_green * (1 - front_alpha);
    float new_blue = front_blue + back_blue * (1 - front_alpha);

    rgba_osd_pixel_t new_color = 0;
    RGBA_W_A(new_color, new_alpha * 255.0f);
    RGBA_W_R(new_color, new_red * 255.0f);
    RGBA_W_G(new_color, new_green * 255.0f);
    RGBA_W_B(new_color, new_blue * 255.0f);

    return new_color;
}

/**
 * @see https://en.wikipedia.org/wiki/Alpha_compositing
 */
static rgba_osd_pixel_t _rgba_osd_overlay_algoarithm_compositing_over_straight(rgba_osd_pixel_t back,
                                                                               rgba_osd_pixel_t front)
{
    float front_alpha = (float)RGBA_R_A(front) / 255.0f;
    float front_red = (float)RGBA_R_R(front) / 255.0f;
    float front_green = (float)RGBA_R_G(front) / 255.0f;
    float front_blue = (float)RGBA_R_B(front) / 255.0f;
    float back_alpha = (float)RGBA_R_A(back) / 255.0f;
    float back_red = (float)RGBA_R_R(back) / 255.0f;
    float back_green = (float)RGBA_R_G(back) / 255.0f;
    float back_blue = (float)RGBA_R_B(back) / 255.0f;

    float new_alpha = front_alpha + back_alpha * (1.0f - front_alpha);
    float new_red = (front_red * front_alpha + back_red * back_alpha * (1 - front_alpha)) / new_alpha;
    float new_green = (front_green * front_alpha + back_green * back_alpha * (1 - front_alpha)) / new_alpha;
    float new_blue = (front_blue * front_alpha + back_blue * back_alpha * (1 - front_alpha)) / new_alpha;

    rgba_osd_pixel_t new_color = 0;
    RGBA_W_A(new_color, new_alpha * 255.0f);
    RGBA_W_R(new_color, new_red * 255.0f);
    RGBA_W_G(new_color, new_green * 255.0f);
    RGBA_W_B(new_color, new_blue * 255.0f);

    return new_color;
}

static rgba_osd_pixel_t _rgba_osd_overlay_algoarithm_replace_all(rgba_osd_pixel_t back, rgba_osd_pixel_t front)
{
    (void)back;
    return front;
}

static rgba_osd_pixel_t _rgba_osd_overlay_algoarithm_replace_non_alpha(rgba_osd_pixel_t back, rgba_osd_pixel_t front)
{
    return RGBA_R_A(front) ? front : back;
}

static void _rgba_osd_overlay_algorithm_fast_template(rgba_osd_pixel_t *canvas, const rgba_osd_size_t *canvas_sz,
                                                      const rgba_osd_size_t          *canvas_pos,
                                                      const rgba_osd_pixel_t         *payload,
                                                      const rgba_osd_size_t          *payload_sz,
                                                      rgba_osd_overlay_compositing_fn compositing)
{
    rgba_osd_size_t render_size = {
        payload_sz->x + canvas_pos->x > canvas_sz->x ? canvas_sz->x - canvas_pos->x : payload_sz->x,
        payload_sz->y + canvas_pos->y > canvas_sz->y ? canvas_sz->y - canvas_pos->y : payload_sz->y,
    };

    rgba_osd_size_t payload_drawing_pos, canvas_drawing_pos;
    for (payload_drawing_pos.y = 0; payload_drawing_pos.y < render_size.y; payload_drawing_pos.y++)
    {
        for (payload_drawing_pos.x = 0; payload_drawing_pos.x < render_size.x; payload_drawing_pos.x++)
        {
            canvas_drawing_pos.x = canvas_pos->x + payload_drawing_pos.x;
            canvas_drawing_pos.y = canvas_pos->y + payload_drawing_pos.y;
            size_t           canvas_offset = canvas_drawing_pos.y * canvas_sz->x + canvas_drawing_pos.x;
            rgba_osd_pixel_t canvas_data = canvas[canvas_offset];
            rgba_osd_pixel_t payload_data = payload[payload_drawing_pos.y * payload_sz->x + payload_drawing_pos.x];
            canvas[canvas_offset] = compositing(canvas_data, payload_data);
        }
    }
}

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

rgba_osd_pixel_t *rgba_osd_canvas(rgba_osd_t *osd)
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

    /* Render. */
    algo(osd->new_pixels, &osd->config.size, canvas_pos, payload, payload_sz);
}

void rgba_osd_overlay_algorithm_compositing_over_premultiplied(rgba_osd_pixel_t       *canvas,
                                                               const rgba_osd_size_t  *canvas_sz,
                                                               const rgba_osd_size_t  *canvas_pos,
                                                               const rgba_osd_pixel_t *payload,
                                                               const rgba_osd_size_t  *payload_sz)
{
    _rgba_osd_overlay_algorithm_fast_template(canvas, canvas_sz, canvas_pos, payload, payload_sz,
                                              _rgba_osd_overlay_algoarithm_compositing_over_premultiplied);
}

void rgba_osd_overlay_algorithm_compositing_over_straight(rgba_osd_pixel_t *canvas, const rgba_osd_size_t *canvas_sz,
                                                          const rgba_osd_size_t  *canvas_pos,
                                                          const rgba_osd_pixel_t *payload,
                                                          const rgba_osd_size_t  *payload_sz)
{
    _rgba_osd_overlay_algorithm_fast_template(canvas, canvas_sz, canvas_pos, payload, payload_sz,
                                              _rgba_osd_overlay_algoarithm_compositing_over_straight);
}

void rgba_osd_overlay_algorithm_replace_all(rgba_osd_pixel_t *canvas, const rgba_osd_size_t *canvas_sz,
                                            const rgba_osd_size_t *canvas_pos, const rgba_osd_pixel_t *payload,
                                            const rgba_osd_size_t *payload_sz)
{
    _rgba_osd_overlay_algorithm_fast_template(canvas, canvas_sz, canvas_pos, payload, payload_sz,
                                              _rgba_osd_overlay_algoarithm_replace_all);
}

void rgba_osd_overlay_algorithm_replace_non_alpha(rgba_osd_pixel_t *canvas, const rgba_osd_size_t *canvas_sz,
                                                  const rgba_osd_size_t *canvas_pos, const rgba_osd_pixel_t *payload,
                                                  const rgba_osd_size_t *payload_sz)
{
    _rgba_osd_overlay_algorithm_fast_template(canvas, canvas_sz, canvas_pos, payload, payload_sz,
                                              _rgba_osd_overlay_algoarithm_replace_non_alpha);
}
