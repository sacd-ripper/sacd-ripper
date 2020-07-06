# CMake build file for SACD Extract

cmake_minimum_required(VERSION 2.6)

# Mingw-w64 support
OPTION(MINGW64 "Mingw-w64" NO)
if (MINGW64 MATCHES "YES")
    MESSAGE(STATUS "Mingw-w64 mode enabled")
    SET(CMAKE_SYSTEM_NAME Windows)
    SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
    SET(TOOLCHAIN_PREFIX x86_64-w64-mingw32)
    SET(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
    SET(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
endif (MINGW64 MATCHES "YES")

execute_process(
    COMMAND git describe --tags --dirty --abbrev=64
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    RESULT_VARIABLE GIT_COMMIT_HASH_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND git remote get-url origin
    OUTPUT_VARIABLE GIT_REPO_URL
    RESULT_VARIABLE GIT_REPO_URL_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Obtain git commit hash and repo url
if(${GIT_COMMIT_HASH_RESULT} GREATER 0)
    add_definitions("-DGIT_COMMIT_HASH=NA")
else(${GIT_COMMIT_HASH_RESULT} GREATER 0)
    add_definitions("-DGIT_COMMIT_HASH=${GIT_COMMIT_HASH}")
    MESSAGE(STATUS "git commit hash: ${GIT_COMMIT_HASH}")
endif(${GIT_COMMIT_HASH_RESULT} GREATER 0)

if(${GIT_REPO_URL_RESULT} GREATER 0)
    add_definitions("-DGIT_REPO_URL=NA")
else(${GIT_REPO_URL_RESULT} GREATER 0)
    add_definitions("-DGIT_REPO_URL=\"${GIT_REPO_URL}\"")
    MESSAGE(STATUS "git repo url: ${GIT_REPO_URL}")
endif(${GIT_REPO_URL_RESULT} GREATER 0)

project(sacd_extract C)

# Load some macros.
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

# Macros we'll need
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)
include(FindThreads)

# Include directory paths
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${sacd_extract_SOURCE_DIR})

include_directories("../../libs/libcommon")
include_directories("../../libs/libdstdec")
include_directories("../../libs/libid3")
include_directories("../../libs/libsacd")

message(STATUS "libmxl2 ${LIBXML2_LIBRARIES}")
find_package(LibXml2 REQUIRED)

# Extra flags for GCC
STRING(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if(NOT CMAKE_BUILD_TYPE_UPPER STREQUAL "DEBUG")
  if (CMAKE_COMPILER_IS_GNUCC OR (CMAKE_C_COMPILER_ID MATCHES "Clang"))
    add_definitions(
        -pipe
        -Wall -Wextra -Wcast-align -Wpointer-arith -O3
        -Wno-unused-parameter `xml2-config --cflags --libs`)
  endif ()
  if(NOT CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")
    add_definitions(
        -msse2)
  endif ()
endif ()


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

if(WIN32)
    set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIRARIES} -lpthread -lws2_32 -liconv -lxml2 -static")
    target_compile_options(${PROJECT_NAME} PRIVATE -municode)
    target_link_libraries(${PROJECT_NAME} -municode)
    add_definitions(-D_FILE_OFFSET_BITS=64)
    add_definitions(-DUNICODE -D_UNICODE -DIN_LIBXML)
elseif(APPLE)
  set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIRARIES} -liconv -lpthread -lxml2")
else()
  add_definitions(-D_FILE_OFFSET_BITS=64)
  set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIRARIES} -lpthread -lxml2")
endif()
