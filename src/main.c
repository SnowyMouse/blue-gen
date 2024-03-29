/*
 * blue-gen (c) 2019 Kavawuvi
 *
 * This program is free software under the GNU General Public License v3.0 or later. See LICENSE for more information.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <ctype.h>
#include "bluegen.h"

typedef struct TIFFTag {
    uint16_t type;
    uint16_t size;
    uint32_t count;
    uint32_t data_offset;
} TIFFTag;

static const uint16_t BITS_PER_SAMPLE[4] = { 0x8, 0x8, 0x8, 0x8 };

bool ends_with(const char *str, const char *ext) {
    size_t str_len = strlen(str);
    size_t ext_len = strlen(ext);

    if(str_len < ext_len) {
        return false;
    }
    else {
        for(const char *i = str + str_len - ext_len, *j = ext; *i; i++, j++) {
            if(tolower(*i) != tolower(*j)) {
                return false;
            }
        }

        return true;
    }
}

int main(int argc, char **argv) {
    int longindex = 0, opt;

    BlueGenPixel dummy_color = { 0x00, 0xFF, 0xFF, 0xFF };

    char *program = argv[0];

    static struct option options[] = {
        {"help",  no_argument, 0, 'h'},
        {"dummy-space",  required_argument, 0, 'd'},
        {0, 0, 0, 0 }
    };

    int first_sequence = 1;
    while(first_sequence < argc && strcmp(argv[first_sequence], "-s") != 0) {
        first_sequence++;
    }

    // Go through each argument
    while((opt = getopt_long(first_sequence, argv, "hd:", options, &longindex)) != -1) {
        switch(opt) {
            case 'd':
                for(char *c = optarg; *c; c++) {
                    *c = tolower(*c);
                }

                unsigned int r,g,b;
                int q = sscanf(optarg, "%02x%02x%02x", &r, &g, &b);

                dummy_color.red = (uint8_t)r;
                dummy_color.green = (uint8_t)g;
                dummy_color.blue = (uint8_t)b;

                if(q != 3) {
                    fprintf(stderr, "(v)> Dummy color must be a valid hex code (i.e. 00FFFF).\n");
                    return 1;
                }
                break;

            case 'h':
            case 0:
                FAIL_HELP:
                fprintf(stderr, "Usage: %s [options] <output> -s <s1image1> [s1image2 ...] [-s <s2image1> ...]\n", program);
                fprintf(stderr, "Takes tiff, png, bmp, and tga images as sequences (-s) and turns them into a\n");
                fprintf(stderr, "valid sprite plate to be compiled into a Halo bitmap.\n\n");
                fprintf(stderr, "Options:\n");
                fprintf(stderr, "    --dummy-space,-d <color>   Set the color of the dummy space (normally cyan)\n");
                fprintf(stderr, "                               via hex code. Default: 00FFFF (RRGGBB)\n");
                fprintf(stderr, "    --help,-h                  Show help\n\n");
                return 1;
        }
    }

    if(first_sequence == argc || optind == first_sequence) {
        goto FAIL_HELP;
    }

    const char *output_path = argv[first_sequence - 1];

    // Figure out how many sequences we have
    size_t sequence_count = 0;
    for(int i = first_sequence; i < argc; i++) {
        if(strcmp(argv[i], "-s") == 0) {
            sequence_count++;
        }
    }

    if(sequence_count == 0) {
        char *new_argv[] = {program, "-h"};
        return main(2, new_argv);
    }

    // Allocate sequences
    BlueGenImageSequence *sequences = malloc(sequence_count * sizeof(*sequences));
    int i = first_sequence + 1;
    for(size_t s = 0; s < sequence_count; s++) {
        BlueGenImageSequence *sequence = sequences + s;
        sequence->image_count = 0;

        // Get all of the images in the sequence
        for(int is = i; is < argc; is++) {
            if(strcmp(argv[is], "-s") == 0) {
                break;
            }
            sequence->image_count++;
        }

        // Allocate images
        sequence->images = malloc(sequence->image_count * sizeof(*sequence->images));
        for(int q = 0; q < sequence->image_count; q++) {
            const char *file = argv[i + q];
            if(ends_with(file, ".tif") || ends_with(file, ".tiff")) {
                load_tiff(sequence->images + q, file);
            }
            else if(ends_with(file, ".png") || ends_with(file, ".tga") || ends_with(file, ".bmp")) {
                load_image(sequence->images + q, file);
            }
            else {
                fprintf(stderr, "(v)> Failed to open %s! Unknown file type...\n", file);
                return 1;
            }
        }
        i += 1 + sequence->image_count;
    }

    BlueGenImage output_image;
    generate_bluegen_image(sequences, sequence_count, &dummy_color, &output_image);

    FILE *f = fopen(output_path, "wb");
    if(!f) {
        fprintf(stderr, "(v)> Failed to open %s for writing.\n", output_path);
        return 1;
    }

    uint16_t magic = 0x4949;
    uint16_t version = 42;

    // Write the TIFF header
    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&version, sizeof(magic), 1, f);

    uint32_t width = output_image.width;
    uint32_t height = output_image.height;

    uint32_t pixel_offset = sizeof(uint32_t) + ftell(f);
    uint32_t tag_offset = width * height * sizeof(*output_image.pixels) + pixel_offset;

    // Write the offset to the tags
    fwrite(&tag_offset, sizeof(tag_offset), 1, f);

    // Write all the pixels
    fwrite(output_image.pixels, output_image.height * output_image.width * sizeof(*output_image.pixels), 1, f);

    // Write however many tags we need
    uint16_t tag_count = 10;
    fwrite(&tag_count, sizeof(tag_count), 1, f);

    uint32_t after_tag_offset = tag_offset + sizeof(tag_count) + sizeof(TIFFTag) * tag_count + 4;

    // Write the width and height
    {
        TIFFTag width_tag;
        width_tag.type = 0x100;
        width_tag.data_offset = width;
        width_tag.size = width >= UINT16_MAX ? 4 : 3;
        width_tag.count = 1;

        TIFFTag height_tag;
        height_tag.type = 0x101;
        height_tag.data_offset = height;
        height_tag.size = height >= UINT16_MAX ? 4 : 3;
        height_tag.count = 1;

        fwrite(&width_tag, sizeof(width_tag), 1, f);
        fwrite(&height_tag, sizeof(height_tag), 1, f);
    }

    // Write the bits per sample
    {
        TIFFTag bits_per_sample_tag;
        bits_per_sample_tag.type = 0x102;
        bits_per_sample_tag.size = 3;
        bits_per_sample_tag.count = 4;
        bits_per_sample_tag.data_offset = after_tag_offset;
        fwrite(&bits_per_sample_tag, sizeof(bits_per_sample_tag), 1, f);

        after_tag_offset += sizeof(BITS_PER_SAMPLE);
    }

    // Write the compression (1 = no compression)
    {
        TIFFTag compression_tag;
        compression_tag.type = 0x103;
        compression_tag.size = 3;
        compression_tag.count = 1;
        compression_tag.data_offset = 1;
        fwrite(&compression_tag, sizeof(compression_tag), 1, f);

        after_tag_offset += sizeof(BITS_PER_SAMPLE);
    }

    // Write the photometric interpretation (2 = RGB)
    {
        TIFFTag photometric_interpretation_tag;
        photometric_interpretation_tag.type = 0x106;
        photometric_interpretation_tag.size = 3;
        photometric_interpretation_tag.count = 1;
        photometric_interpretation_tag.data_offset = 2;
        fwrite(&photometric_interpretation_tag, sizeof(photometric_interpretation_tag), 1, f);

        after_tag_offset += sizeof(BITS_PER_SAMPLE);
    }

    // Write the strips offset
    {
        TIFFTag strips_tag;
        strips_tag.type = 0x111;
        strips_tag.data_offset = pixel_offset;
        strips_tag.size = 4;
        strips_tag.count = 1;
        fwrite(&strips_tag, sizeof(strips_tag), 1, f);
    }

    // Write the orientation (1 = top-left)
    {
        TIFFTag strips_tag;
        strips_tag.type = 0x112;
        strips_tag.data_offset = 1;
        strips_tag.size = 3;
        strips_tag.count = 1;
        fwrite(&strips_tag, sizeof(strips_tag), 1, f);
    }

    // Write the samples per pixel (4 samples per pixel, rgba)
    {
        TIFFTag samples_per_pixel_tag;
        samples_per_pixel_tag.type = 0x115;
        samples_per_pixel_tag.data_offset = 4;
        samples_per_pixel_tag.size = 3;
        samples_per_pixel_tag.count = 1;
        fwrite(&samples_per_pixel_tag, sizeof(samples_per_pixel_tag), 1, f);
    }

    // Write the strip byte counts
    {
        TIFFTag strip_byte_count_tag;
        strip_byte_count_tag.type = 0x117;
        strip_byte_count_tag.data_offset = width * height * sizeof(*output_image.pixels);
        strip_byte_count_tag.size = 4;
        strip_byte_count_tag.count = 1;
        fwrite(&strip_byte_count_tag, sizeof(strip_byte_count_tag), 1, f);
    }

    // Write the extra samples (2 = unassociated alpha)
    {
        TIFFTag extra_samples_tag;
        extra_samples_tag.type = 0x152;
        extra_samples_tag.data_offset = 2;
        extra_samples_tag.size = 3;
        extra_samples_tag.count = 1;
        fwrite(&extra_samples_tag, sizeof(extra_samples_tag), 1, f);
    }

    // Next directory offset
    uint32_t next_directory_offset = 0;
    fwrite(&next_directory_offset, sizeof(next_directory_offset), 1, f);

    // Write all those bits per sample
    fwrite(BITS_PER_SAMPLE, sizeof(BITS_PER_SAMPLE), 1, f);

    fclose(f);

    fprintf(stdout, "(^)> Yay! I made a %ux%u image.\n", output_image.width, output_image.height);

    return 0;
}
