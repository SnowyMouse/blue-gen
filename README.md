# blue-gen
This program combines multiple TIFF images into a single TIFF color plate for Halo Custom Edition bitmap creation.

The syntax is simple: the paths to your images with sequences separated by `-s`. The last argument is the output image.

By default, blue (`#0000FF`) is used to separate bitmaps and magenta (`#FF00FF`) is used to separate sequences. If any
bitmap uses either color, then some other color unused by your bitmap(s) will be used, instead. If, somehow, you used
up every possible color in the RGB space across all of your images, you will get an error instead.

You will need LibTIFF in order to build and run this program. Otherwise, this program is written in C using the C99
standard.
