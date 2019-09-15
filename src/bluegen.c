/*
 * blue-gen (c) 2019 Kavawuvi
 *
 * This program is free software under the GNU General Public License v3.0 or later. See LICENSE for more information.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <tiffio.h>
#include "bluegen.h"
#include "stb_image.h"

#define BITMAP_SPACING 4
#define SEQUENCE_SPACING 1
#define BLUE_GAP 1
#define COLOR_PLATE_GAP (SEQUENCE_SPACING + BLUE_GAP * 2)

void initialize_bluegen_image(BlueGenImage *image, uint32_t width, uint32_t height) {
    image->pixels = calloc(width * height, sizeof(*image->pixels));
    image->height = height;
    image->width = width;
    image->free = free;
}
void free_bluegen_image(BlueGenImage *image) {
    image->free(image->pixels);
}

// Determine if the color is safe
bool is_safe_color(const BlueGenPixel *pixel, const BlueGenImageSequence *sequences, size_t sequence_count) {
    for(size_t s = 0; s < sequence_count; s++) {
        const BlueGenImageSequence *sequence = sequences + s;
        for(size_t i = 0; i < sequence->image_count; i++) {
            const BlueGenImage *image = sequence->images + i;
            for(size_t y = 0; y < image->height; y++) {
                for(size_t x = 0; x < image->width; x++) {
                    const BlueGenPixel *image_pixel = image->pixels + x + y * image->width;
                    if(pixel->red == image_pixel->red && pixel->green == image_pixel->green && pixel->blue == image_pixel->blue) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void increment_pixel(BlueGenPixel *pixel, const BlueGenPixel *dummy_space) {
    if(pixel->red == 0xFF) {
        pixel->red = 0x00;
        if(pixel->green == 0xFF) {
            pixel->green = 0x00;
            if(pixel->blue == 0xFF) {
                fprintf(stderr, "(O)< Eep! I need two unused colors!\n");
                exit(1);
            }
            else {
                pixel->blue++;
            }
        }
        else {
            pixel->green++;
        }
    }
    else {
        pixel->red++;
    }

    // Skip cyan, blue, and magenta
    if(pixel->green == dummy_space->green && pixel->blue == dummy_space->blue && pixel->red == dummy_space->red) {
        increment_pixel(pixel, dummy_space);
    }
    if(pixel->green == 0x00 && pixel->blue == 0xFF && (pixel->red == 0x00 || pixel->red == 0xFF)) {
        increment_pixel(pixel, dummy_space);
    }
}

void generate_bluegen_image(const BlueGenImageSequence *sequences, size_t sequence_count, const BlueGenPixel *dummy_space, BlueGenImage *output) {
    // Go through each sequence so we can determine how wide and tall to make our image
    size_t width = 4;
    size_t height = 1;
    for(size_t s = 0; s < sequence_count; s++) {
        const BlueGenImageSequence *sequence = sequences + s;
        size_t this_sequence_width = 0;
        size_t this_sequence_height = 0;

        // If we have any images, do stuff
        for(size_t i = 0; i < sequence->image_count; i++) {
            const BlueGenImage *image = sequence->images + i;
            if(image->height > this_sequence_height) {
                this_sequence_height = image->height;
            }
            this_sequence_width += image->width + BITMAP_SPACING;
        }

        if(sequence->image_count > 1) {
            this_sequence_width -= BITMAP_SPACING;
        }

        // Add a couple pixels to the left and write for spacing
        this_sequence_width += 2;

        if(width < this_sequence_width) {
            width = this_sequence_width;
        }

        height += this_sequence_height + COLOR_PLATE_GAP;
    }

    // This is used as a fallback
    BlueGenPixel SAFE_PIXEL = { 0x00, 0x00, 0x00, 0xFF };

    // Next, find some safe colors for blue and magenta
    BlueGenPixel BLUE_PIXEL = { 0x00, 0x00, 0xFF, 0xFF };
    while(!is_safe_color(&BLUE_PIXEL, sequences, sequence_count)) {
        increment_pixel(&SAFE_PIXEL, dummy_space);
        BLUE_PIXEL = SAFE_PIXEL;
    }
    BlueGenPixel MAGENTA_PIXEL = { 0xFF, 0x00, 0xFF, 0xFF };
    while(!is_safe_color(&MAGENTA_PIXEL, sequences, sequence_count)) {
        increment_pixel(&SAFE_PIXEL, dummy_space);
        MAGENTA_PIXEL = SAFE_PIXEL;
    }

    initialize_bluegen_image(output, width, height);

    // First, make the color plate
    output->pixels[0] = BLUE_PIXEL;
    output->pixels[1] = MAGENTA_PIXEL;
    output->pixels[2] = *dummy_space;
    for(uint32_t x = 3; x < width; x++) {
        output->pixels[x] = BLUE_PIXEL;
    }

    // Next, go through each sequence
    uint32_t y = 1;

    #define FILL_LINE(color, amount) for(uint32_t g = 0; g < amount; g++) { for(uint32_t x = 0; x < width; x++) { output->pixels[x + y * width] = color; } y++; }

    for(size_t s = 0; s < sequence_count; s++) {
        const BlueGenImageSequence *sequence = sequences + s;

        // Fill the first two lines with magenta and blue respectively
        FILL_LINE(MAGENTA_PIXEL, SEQUENCE_SPACING);
        FILL_LINE(BLUE_PIXEL, BLUE_GAP);

        // Find out the height of this sequence
        uint32_t sequence_height = 0;
        for(size_t i = 0; i < sequence->image_count; i++) {
            uint32_t image_height = sequence->images[i].height;
            if(image_height > sequence_height) {
                sequence_height = image_height;
            }
        }

        // Fill out the sequence with blue pixels
        FILL_LINE(BLUE_PIXEL, sequence_height);
        y -= sequence_height;

        // Put the images in place
        uint32_t x = 1;
        for(size_t i = 0; i < sequence->image_count; i++) {
            const BlueGenImage *image = sequence->images + i;
            uint32_t image_height = image->height;
            uint32_t image_width = image->width;

            for(uint32_t iy = 0; iy < image_height; iy++) {
                for(uint32_t ix = 0; ix < image_width; ix++) {

                    const BlueGenPixel *input_pixel = image->pixels + ix + iy * image_width;
                    BlueGenPixel *output_pixel = output->pixels + x + ix + (y + iy) * width;
                    *output_pixel = *input_pixel;
                }
            }

            x += BITMAP_SPACING + image->width;
        }

        y += sequence_height;

        // Add one more blue line
        FILL_LINE(BLUE_PIXEL, 1);
    }
}

void load_tiff(BlueGenImage *image, const char *path) {
    // Open the tiff
    TIFF *image_tiff = TIFFOpen(path, "r");
    if(!image_tiff) {
        fprintf(stderr, "(v)> Cannot open TIFF %s\n", path);
        exit(EXIT_FAILURE);
    }

    // Allocate and the image
    uint32_t width, height;
    TIFFGetField(image_tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(image_tiff, TIFFTAG_IMAGELENGTH, &height);
    initialize_bluegen_image(image, width, height);

    // Force associated alpha so alpha doesn't get multiplied in TIFFReadRGBAImageOriented
    uint16_t ua[] = { EXTRASAMPLE_ASSOCALPHA };
    TIFFSetField(image_tiff, TIFFTAG_EXTRASAMPLES, 1, ua);

    // Read it all
    TIFFReadRGBAImageOriented(image_tiff, width, height, (uint32_t *)(image->pixels), ORIENTATION_TOPLEFT, 0);

    // Close the TIFF
    TIFFClose(image_tiff);
}

void load_image(BlueGenImage *image, const char *path) {
    // Load it
    int width, height, channels = 0;
    image->pixels = (BlueGenPixel *)stbi_load(path, &width, &height, &channels, 4);
    if(!image->pixels) {
        fprintf(stderr, "(v)> Failed to load %s! Error was: %s\n", path, stbi_failure_reason());
        exit(EXIT_FAILURE);
    }
    image->width = (uint32_t)(width);
    image->height = (uint32_t)(height);
    image->free = stbi_image_free;
}
