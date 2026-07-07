cmake_minimum_required(VERSION 3.11)
project(loadtracker VERSION 1.99.0 HOMEPAGE_URL https://github.com/libsidplayfp/loadtracker)

set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR
        "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options("$<$<CONFIG:DEBUG>:-Wall;-Wextra;-pedantic>")
endif()

set(LOADTRK_SOURCES
    ${CMAKE_BINARY_DIR}/src/goatdata.c
    src/loadtrk.c
    src/gsong.c
    src/gorder.c
    src/gpattern.c
    src/instr.cpp
    src/gtable.c
    src/gplay.c
    src/display.cpp
    src/file.cpp
    src/greloc.c
    src/help.cpp
    src/gsound.c
    src/console.cpp
    src/gsid.cpp
    src/asm/asmtab.c
    src/asm/chnkpool.c
    src/asm/expr.c
    src/asm/lexyy.c
    src/asm/log.c
    src/asm/membuf.c
    src/asm/membufio.c
    src/asm/namedbuf.c
    src/asm/parse.c
    src/asm/pc.c
    src/asm/vec.c
    src/bme/bme_gfx.c
    src/bme/bme_snd.c
    src/bme/bme_win.c
    src/bme/bme_mou.c
    src/bme/bme_io.c
    src/bme/bme_end.c
    src/bme/bme.c
    src/rtmidi/rtmidi_c.h
    src/rtmidi/rtmidi_c.cpp
    src/rtmidi/RtMidi.h
    src/rtmidi/RtMidi.cpp)

if(MINGW)
    list(APPEND LOADTRK_SOURCES
        src/loadtrk.rc
    )
endif(MINGW)

set(MOD2SNG_SOURCES
    src/mod2sng.c
    src/bme/bme_end.c)

set(INS2SND2_SOURCES
    src/ins2snd2.c
    src/bme/bme_end.c)

set(SNGSPLI2_SOURCES
    src/sngspli2.c
    src/bme/bme_end.c)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/bme)
include_directories(${CMAKE_SOURCE_DIR}/src/asm)
include_directories(${CMAKE_BINARY_DIR}/src/residfp)

find_package(SDL3 REQUIRED)

add_executable(datafile src/bme/datafile.c src/bme/bme_end.c)
add_executable(dat2inc src/bme/dat2inc.c)

target_include_directories(datafile PUBLIC SDL3::Headers)


file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/src)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/src/loadtrk.dat
  COMMAND datafile ${CMAKE_BINARY_DIR}/src/loadtrk.dat loadtrk.seq
  DEPENDS datafile
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src
  )

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/src/goatdata.c
  COMMAND dat2inc loadtrk.dat goatdata.c
  DEPENDS dat2inc ${CMAKE_BINARY_DIR}/src/loadtrk.dat
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
  )

add_executable(loadtrk ${LOADTRK_SOURCES})
add_executable(mod2sng ${MOD2SNG_SOURCES})
add_executable(ins2snd2 ${INS2SND2_SOURCES})
add_executable(sngspli2 ${SNGSPLI2_SOURCES})

add_custom_target(goatdata DEPENDS ${CMAKE_BINARY_DIR}/src/goatdata.c)
add_dependencies(loadtrk goatdata)

target_include_directories(mod2sng PUBLIC SDL3::Headers)
target_include_directories(ins2snd2 PUBLIC SDL3::Headers)
target_include_directories(sngspli2 PUBLIC SDL3::Headers)

if(MINGW)
    set(LIB_MINGW mingw32)
endif()

OPTION(BUILD_STATIC "Static build" OFF)
IF(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    target_link_libraries(loadtrk SDL3::SDL3-static)
ELSE()
    target_link_libraries(loadtrk SDL3::SDL3)
ENDIF()

find_package(Jack)
IF(JACK_FOUND)
    target_compile_definitions(loadtrk PUBLIC USE_JACK __UNIX_JACK__)
    include_directories(${JACK_INCLUDE_DIR})
    target_link_libraries(loadtrk ${JACK_LIBRARY})
    set(use_midi ON)
ENDIF(JACK_FOUND)

find_package(ALSA)
if (ALSA_FOUND)
    target_compile_definitions(loadtrk PUBLIC __LINUX_ALSA__)
    include_directories(${ALSA_INCLUDE_DIR})
    target_link_libraries(loadtrk ${ALSA_LIBRARY} pthread)
    set(use_midi ON)
endif(ALSA_FOUND)

if(APPLE)
    find_library(COREMIDI CoreMIDI)
    find_library(COREAUDIO CoreAudio)
    find_library(COREFOUNDATION CoreFoundation)
    target_compile_definitions(loadtrk PUBLIC __MACOSX_CORE__)
    target_link_libraries(loadtrk ${COREMIDI} ${COREAUDIO} ${COREFOUNDATION} pthread)
    set(use_midi ON)
endif(APPLE)

if(WIN32 OR MINGW)
    target_compile_definitions(loadtrk PUBLIC __WINDOWS_MM__)
    target_link_libraries(loadtrk winmm)
    set(use_midi ON)
endif()

if (use_midi)
    target_compile_definitions(loadtrk PUBLIC USE_MIDI_INPUT)
endif()

find_package(PkgConfig REQUIRED)

pkg_check_modules(EXSID IMPORTED_TARGET libexsid>=2.0)
if (EXSID_FOUND)
    target_compile_definitions(loadtrk PUBLIC USE_EXSID)
    target_link_libraries(loadtrk PkgConfig::EXSID)
endif()


pkg_check_modules(RESIDFP REQUIRED IMPORTED_TARGET libresidfp>=0.1)
if(BUILD_STATIC)
    target_link_libraries(loadtrk PkgConfig::RESIDFP-static)
else()
    target_link_libraries(loadtrk PkgConfig::RESIDFP)
endif()

##################################################################
# Check for LTO

OPTION(LTO "Enable link time optimization" ON)
if(LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT have_lto OUTPUT output LANGUAGES CXX)

    if (have_lto)
        message(STATUS "LTO enabled")
        set_property(TARGET loadtrk
             PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "LTO is not supported: ${output}")
    endif()
endif()

#####################################################

include(GNUInstallDirs)

INSTALL(TARGETS loadtrk RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES loadtrk.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
INSTALL(FILES loadtrk.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons)
INSTALL(DIRECTORY examples DESTINATION ${CMAKE_INSTALL_DATADIR}/loadtrk FILES_MATCHING REGEX ".*\.(sng|ins)")
