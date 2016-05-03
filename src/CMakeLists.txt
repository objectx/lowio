#
# Copyright (c) 2016 Masashi Fujita
#

cmake_minimum_required (VERSION 3.5)

include (CheckIncludeFileCXX)
include (CheckFunctionExists)

check_include_file_cxx (stdint.h HAVE_STDINT_H)
check_include_file_cxx (unistd.h HAVE_UNISTD_H)
check_include_file_cxx (fcntl.h HAVE_FCNTL_H)
check_include_file_cxx (sys/stat.h HAVE_SYS_STAT_H)
check_function_exists (lseek64 HAVE_LSEEK64)

set (TARGET_NAME lowio)

set (LOWIO_SOURCES lowio.cpp)
set (LOWIO_HEADERS "${LOWIO_SOURCE_DIR}/include/lowio/lowio.hpp")

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/config.hpp")

add_library (${TARGET_NAME} STATIC ${LOWIO_SOURCES} ${LOWIO_HEADERS})
    target_include_directories (${TARGET_NAME}
                                PUBLIC $<BUILD_INTERFACE:${LOWIO_SOURCE_DIR}/include>
                                       $<INSTALL_INTERFACE:include>
                                PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_definitions (${TARGET_NAME} PRIVATE "-DHAVE_CONFIG_HPP")
    target_compile_features (${TARGET_NAME} PUBLIC cxx_auto_type
                                                   cxx_constexpr
                                                   cxx_generalized_initializers)
