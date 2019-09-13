/*
 * blue-gen (c) 2019 Kavawuvi
 *
 * This program is free software under the GNU General Public License v3.0 or later. See LICENSE for more information.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A single color, holding values for four channels: red, green, blue, and alpha
 */
typedef struct BlueGenPixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} BlueGenPixel;

typedef struct BlueGenImage {
    /** Holds a pointer to pixel data */
    BlueGenPixel *pixels;

    /** Width of the image in pixels */
    uint32_t width;

    /** Height of the image in pixels */
    uint32_t height;
} BlueGenImage;

typedef struct BlueGenImageSequence {
    /** Holds a pointer to an array of images */
    BlueGenImage *images;

    /** Number of images */
    size_t image_count;
} BlueGenImageSequence;

/**
 * Initialize a blank image
 * @param image  pointer to a struct to hold image data
 * @param width  width of image in pixels
 * @param height height of image in pixels
 */
void initialize_bluegen_image(BlueGenImage *image, uint32_t width, uint32_t height);

/**
 * Generate an image from sequences
 * @param sequences      sequence to generate image from
 * @param sequence_count number of sequences to generate image from
 * @param output         output image
 */
void generate_bluegen_image(const BlueGenImageSequence *sequences, size_t sequence_count, BlueGenImage *output);

/**
 * Load a TIFF at the given path
 * @param image image to load to
 * @param path  path to read from
 */
void load_tiff(BlueGenImage *image, const char *path);

/**
 * Free an image; This is required to prevent memory leakage
 * @param image pointer to BlueGenImage struct
 */
void free_bluegen_image(BlueGenImage *image);

#ifdef __cplusplus
}
#endif
