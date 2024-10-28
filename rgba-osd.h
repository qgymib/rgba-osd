#ifndef RGBA_OSD_H
#define RGBA_OSD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RGBA8888.
 * R is at the lowest address, G after it, B after that, and A last.
 * On a little endian architecture this is equivalent to ABGR32.
 */
typedef uint32_t rgba_osd_pixel_t;

/**
 * @brief OSD overlay handle.
 */
typedef struct rgba_osd rgba_osd_t;

#define RGBA_R_R(c) ((uint32_t)(c) & 0x000000FF)
#define RGBA_R_G(c) (((uint32_t)(c) & 0x0000FF00) >> 0x08)
#define RGBA_R_B(c) (((uint32_t)(c) & 0x00FF0000) >> 0x10)
#define RGBA_R_A(c) (((uint32_t)(c) & 0xFF000000) >> 0x18)

#define RGBA_W_R(c, r) ((c) |= (uint32_t)(r) & 0x000000FF)
#define RGBA_W_G(c, g) ((c) |= ((uint32_t)(g) << 0x08) & 0x0000FF00)
#define RGBA_W_B(c, b) ((c) |= ((uint32_t)(b) << 0x10) & 0x00FF0000)
#define RGBA_W_A(c, a) ((c) |= ((uint32_t)(a) << 0x18) & 0xFF000000)

typedef struct rgba_osd_size
{
    /**
     * @brief The width of the object.
     */
    size_t x;

    /**
     * @brief The height of the object.
     */
    size_t y;
} rgba_osd_size_t;

typedef struct rgba_osd_config
{
    /**
     * @brief The size of the canvas.
     * The upper-left corner of the canvas has the coordinates (0,0).
     */
    rgba_osd_size_t size;

    /**
     * @brief Memory allocates replacement.
     * @see https://linux.die.net/man/3/realloc
     */
    void *(*realloc)(void *ptr, size_t size);
} rgba_osd_config_t;

/**
 * @brief The pixel overlay algorithm function.
 * @param[in] canvas        Canvas pixels.
 * @param[in] canvas_sz     The size of canvas (in pixels).
 * @param[in] canvas_pos    The pixel position on canvas.
 * @param[in] payload       The payload pixels.
 * @param[in] payload_sz    The size of payload (in pixels).
 * @param[in] payload_pos   The pixel position on payload.
 * @return The pixel draw on canvas.
 */
typedef rgba_osd_pixel_t (*rgba_osd_overlay_algorithm_fn)(
    const rgba_osd_pixel_t *canvas, const rgba_osd_size_t *canvas_sz, const rgba_osd_size_t *canvas_pos,
    const rgba_osd_pixel_t *payload, const rgba_osd_size_t *payload_sz, const rgba_osd_size_t *payload_pos);

/**
 * @brief Initialize a osd overlay handle.
 * @param[out] osd  The created handle.
 * @param[in] config    Configuration.
 * @return 0 if create success.
 * @return An negative error number.
 */
int rgba_osd_init(rgba_osd_t **osd, const rgba_osd_config_t *config);

/**
 * @brief Desktroy a osd overlay handle.
 * @param[in] osd   The OSD handle.
 */
void rgba_osd_exit(rgba_osd_t *osd);

/**
 * @brief Get canvas pixels.
 * @param[in] osd   The OSD handle.
 * @return Canvas pixels.
 */
rgba_osd_pixel_t *rgba_osd_pixels(rgba_osd_t *osd);

/**
 * @brief Get canvas size.
 * @param[in] osd   The OSD handle.
 * @return Canvas size.
 */
rgba_osd_size_t *rgba_osd_canvas_size(rgba_osd_t *osd);

/**
 * @brief Start a new frame.
 * @param[in] osd   The OSD handle.
 */
void rgba_osd_new_frame(rgba_osd_t *osd);

/**
 * @brief End the frame and render it.
 * @param[in] osd   The OSD handle.
 */
void rgba_osd_end_frame(rgba_osd_t *osd);

/**
 * @brief Render the \p pixels with size \p size on canvas with position \p pos.
 * @param[in] osd           The OSD handle.
 * @param[in] payload       Payload pixels.
 * @param[in] payload_sz    Width and height of the payload, in pixels.
 * @param[in] canvas_pos    The render position.
 * @param[in] algo          Overlay algorithm. Available algorithms are:
 *   + #rgba_osd_overlay_algorithm_replace_all().
 *   + #rgba_osd_overlay_algorithm_replace_non_alpha().
 */
void rgba_osd_overlay(rgba_osd_t *osd, const rgba_osd_pixel_t *payload, const rgba_osd_size_t *payload_sz,
                      const rgba_osd_size_t *canvas_pos, rgba_osd_overlay_algorithm_fn algo);

/**
 * @brief Replace all pixles with \p payload.
 */
rgba_osd_pixel_t rgba_osd_overlay_algorithm_replace_all(
    const rgba_osd_pixel_t *canvas, const rgba_osd_size_t *canvas_sz, const rgba_osd_size_t *canvas_pos,
    const rgba_osd_pixel_t *payload, const rgba_osd_size_t *payload_sz, const rgba_osd_size_t *payload_pos);

/**
 * @brief Replace pixles that has non-zero alpha channel.
 * If the alpha channel in \p payload is non-zero, replace the original pixles on \p canvas.
 */
rgba_osd_pixel_t rgba_osd_overlay_algorithm_replace_non_alpha(
    const rgba_osd_pixel_t *canvas, const rgba_osd_size_t *canvas_sz, const rgba_osd_size_t *canvas_pos,
    const rgba_osd_pixel_t *payload, const rgba_osd_size_t *payload_sz, const rgba_osd_size_t *payload_pos);

#ifdef __cplusplus
}
#endif
#endif
