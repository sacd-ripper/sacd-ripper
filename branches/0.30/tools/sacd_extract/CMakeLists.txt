# CMake build file for SACD Extract

cmake_minimum_required(VERSION 2.6)
project(sacd_extract C)

# Macros we'll need
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)
include(FindThreads)

# Include directory paths
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${sacd_extract_SOURCE_DIR})

if (MSVC)
    include_directories("./win32")
endif (MSVC)
include_directories("../../libs/libcommon")
include_directories("../../libs/libdstdec")
include_directories("../../libs/libid3")
include_directories("../../libs/libsacd")

# Extra flags for GCC
if (CMAKE_COMPILER_IS_GNUCC)
  add_definitions(
      -pipe
      -Wall -Wextra -Wcast-align -Wpointer-arith
      -Wno-unused-parameter -msse2)
endif (CMAKE_COMPILER_IS_GNUCC)

if (MSVC)
    SET (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    SET (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT /D PTW32_STATIC_LIB")
    ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_DEPRECATE)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
    ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_WARNINGS)
    SET (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ws2_32.lib pthreadVC2.lib iconv.lib")
    SET (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ws2_32.lib pthreadVC2_static.lib iconv.lib /NODEFAULTLIB:LIBCMT.LIB") 

    # disable warnings about libiconv link directory
    cmake_policy(SET CMP0015 NEW)

    include_directories("../../libs/libiconv/include")
    link_directories("../../libs/libiconv/lib/")
elseif(WIN32)
  set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIRARIES} -lpthread -lws2_32 -liconv")
else()
  set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIRARIES} -lpthread")
endif()

file(GLOB libcommon_headers ../../libs/libcommon/*.h)
file(GLOB libcommon_sources ../../libs/libcommon/*.c)
source_group(libcommon FILES ${libcommon_headers} ${libcommon_sources})

file(GLOB libdstdec_headers ../../libs/libdstdec/*.h)
file(GLOB libdstdec_sources ../../libs/libdstdec/*.c)
source_group(libdstdec FILES ${libdstdec_headers} ${libdstdec_sources})

file(GLOB libid3_headers ../../libs/libid3/*.h)
file(GLOB libid3_sources ../../libs/libid3/*.c)
source_group(libid3 FILES ${libid3_headers} ${libid3_sources})

file(GLOB libsacd_headers ../../libs/libsacd/*.h)
file(GLOB libsacd_sources ../../libs/libsacd/*.c)
source_group(libsacd FILES ${libsacd_headers} ${libsacd_sources})

file(GLOB main_headers ./*.h)
file(GLOB main_sources ./*.c)
source_group(main FILES ${main_headers} ${main_sources})

add_executable(sacd_extract 
    ${main_headers} ${main_sources}
    ${libcommon_headers} ${libcommon_sources}
    ${libdstdec_headers} ${libdstdec_sources}
    ${libid3_headers} ${libid3_sources}
    ${libsacd_headers} ${libsacd_sources}
    )
