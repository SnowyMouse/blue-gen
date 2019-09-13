#
# blue-gen (c) 2019 Kavawuvi
#
# This program is free software under the GNU General Public License v3.0 or later. See LICENSE for more information.
#

# Require at least version 3.10
cmake_minimum_required(VERSION 3.10)

# Project name - You should change this if you're making a fork
project(blue-gen
    VERSION 0.1.0
    DESCRIPTION "Color plate generator for Halo Custom Edition"
    LANGUAGES C CXX
)

# Use C99
set(CMAKE_C_STANDARD 99)

# Find some packages
find_package(TIFF)

if(${TIFF_FOUND})
    add_executable(blue-gen
        src/main.c
        src/bluegen.c
    )
    target_link_libraries(blue-gen ${TIFF_LIBRARIES})
    target_include_directories(blue-gen
        PUBLIC ${TIFF_INCLUDE_DIRS}
    )
else()
    message(FATAL_ERROR "Failed to find LibTIFF. This is required to make blue-gen.")
endif()
