#
# blue-gen (c) 2019 Kavawuvi
#
# This program is free software under the GNU General Public License v3.0 or later. See LICENSE for more information.
#

# Require at least version 3.12
cmake_minimum_required(VERSION 3.12)

# Project name - You should change this if you're making a fork
project(blue-gen
    VERSION 0.5.0
    DESCRIPTION "Color plate generator for Halo Custom Edition"
    LANGUAGES C CXX
)

# Use C99
set(CMAKE_C_STANDARD 99)

# Use C++17
set(CMAKE_CXX_STANDARD 17)

# Option to build "blue-genstone" Qt GUI
option(BUILD_QT_GUI "Enable Qt GUI frontend for blue-gen" OFF)

# Find some packages
find_package(TIFF)

if(${TIFF_FOUND})
    add_executable(blue-gen
        src/main.c
        src/bluegen.c
        src/stb_impl.c
    )

    if(WIN32)
        # Add Windows resource file
        target_sources(blue-gen PRIVATE src/windows.rc)

        # Just do it.
        if(MINGW)
            set(TIFF_LIBRARIES ${TIFF_LIBRARIES} jpeg lzma z)
        endif()
    endif()

    target_link_libraries(blue-gen ${TIFF_LIBRARIES})
    target_include_directories(blue-gen
        PUBLIC ${TIFF_INCLUDE_DIRS}
    )
else()
    message(FATAL_ERROR "Failed to find LibTIFF. This is required to make blue-gen.")
endif()

if(BUILD_QT_GUI)
    find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    set(BLUE_GENSONE_LIBRARIES Qt6::Widgets)

    add_executable(blue-genstone
        blue-genstone/aboutdialog.cpp
        blue-genstone/main.cpp
        blue-genstone/bluegenstone.cpp
        blue-genstone/universal.qrc
    )

    if(WIN32)
        # Add Windows resource file
        target_sources(blue-genstone PRIVATE blue-genstone/windows.rc)

        # Don't spawn a CMD window when you open blue-genstone
        set(BLUE_GENSONE_LIBRARIES ${BLUE_GENSONE_LIBRARIES} -mwindows)

        # Required for FORTIFY_SOURCES
        if(MINGW)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fstack-protector")
            set(BLUE_GENSONE_LIBRARIES ${BLUE_GENSONE_LIBRARIES} ssp)
        endif()
    endif()

    target_link_libraries(blue-genstone ${BLUE_GENSONE_LIBRARIES})
    target_include_directories(blue-genstone PUBLIC ${Qt6Widgets_INCLUDE_DIRS})
endif()
