# blue-gen
This program combines multiple TIFF, PNG, BMP, and TGA images into a single TIFF color plate for Halo Custom Edition
bitmap creation.

The syntax is simple: First include any options. Then, include your sequences. Sequences start with `-s` with each
argument after that being the path to each bitmap.

By default, blue (`0000FF`) is used to separate bitmaps and magenta (`FF00FF`) is used to separate sequences. If any
bitmap uses either color, then some other color unused by your bitmap(s) will be used, instead. If, somehow, you used
up every possible color in the RGB space across all of your images, you will get an error instead.

It is recommended that each image in a sequence be the same height, especially if you're using sprites. This is to
ensure the registration point tool.exe calculates will be what you expect. You can use dummy space (`00FFFF`) to
increase the dimensions of an image without affecting the size of the bitmap tool.exe creates.

You will need LibTIFF in order to build and run this program. Otherwise, this program is written in C using the C99
standard.
