# toolchain.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Specify the cross compiler
set(CMAKE_C_COMPILER ${RDK_TOOLCHAIN_PATH}/bin/${TOOL_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${RDK_TOOLCHAIN_PATH}/bin/${TOOL_PREFIX}-g++)
set(CMAKE_AR "${RDK_TOOLCHAIN_PATH}/bin/${TOOL_PREFIX}-ar" CACHE FILEPATH "Archiver")
set(CMAKE_AS "${RDK_TOOLCHAIN_PATH}/bin/${TOOL_PREFIX}-as" CACHE FILEPATH "Assembler")
set(CMAKE_RANLIB "${RDK_TOOLCHAIN_PATH}/bin/${TOOL_PREFIX}-ranlib" CACHE FILEPATH "Ranlib")
set(CMAKE_LD "${RDK_TOOLCHAIN_PATH}/bin/${TOOL_PREFIX}-ld" CACHE FILEPATH "Linker")

# Specify the sysroot
set(CMAKE_FIND_ROOT_PATH ${RDK_TOOLCHAIN_PATH}/${TOOL_PREFIX}/libc)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# For libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#-rwxr-xr-x  1 vdubey200 vdubey200  818K Apr 19 01:10 libmodel_processing.so
#-rwxr-xr-x  1 vdubey200 vdubey200  417K Apr 19 01:10 libframe_processing.so