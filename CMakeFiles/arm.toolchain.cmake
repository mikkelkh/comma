# Usage:  ccmake -DCMAKE_TOOLCHAIN_FILE=pathToThisFile pathToSourceRoot

# in cmake files in your source tree/libs, if you need to check for this toolchain, use this...
# IF(CMAKE_CXX_COMPILER MATCHES "arm-linux-uclibcgnueabi-g[+][+]")

# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

IF( "${ARM_ARCH}" STREQUAL "aarch64" )
    # Specify the cross compiler
    # For arm64-v8 specifically, this does not work for Android which has no libc.so
    SET( CMAKE_C_COMPILER   aarch64-linux-android-gcc )
    SET( CMAKE_CXX_COMPILER aarch64-linux-android-g++ )
    set(ARM_LINUX_SYSROOT /stage/android CACHE PATH "ARM cross compilation system root")
ELSE()
    # specify the cross compiler
    SET( CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc )
    SET( CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++ )
    set(ARM_LINUX_SYSROOT /usr/arm-linux-gnueabihf/ CACHE PATH "ARM cross compilation system root")
ENDIF()

set(ARM_STAGE_ROOT /stage/arm/ CACHE PATH "ARM cross compilation user build root")
set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${ARM_LINUX_SYSROOT})
set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${ARM_STAGE_ROOT} )

# where is the target environment 
#SET(CMAKE_FIND_ROOT_PATH  ${RED_PITAYA_ROOT_PATH})

# search for programs in the build host directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directoriesccma
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
