
cmake_minimum_required (VERSION 3.10)

# include (${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

set (SOURCE_FILES
     common.hpp
     openclose.cpp
     handle.cpp
     output.cpp
     fixture.cpp
     fixture.hpp
     parse_flags.cpp
     main.cpp
)

# add_compile_options (-std=gnu++14)
set_source_files_properties (main.cpp PROPERTIES COTIRE_EXCLUDED ON)

function (make_test name_)
    add_executable (${name_} pch.cpp ${SOURCE_FILES})
    conan_basic_setup (TARGETS)
    target_link_libraries (${name_} PRIVATE lowio CONAN_PKG::boost_filesystem CONAN_PKG::catch2)

    set_target_properties (${name_} PROPERTIES COTIRE_PREFIX_HEADER_INCLUDE_PATH "${LOWIO_SOURCE_DIR}/ext")

    if (COMMAND cotire)
        set_target_properties (${name_} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "common.hpp")
        cotire (${name_})
    endif ()
    add_test (NAME ${name_}
              COMMAND ${name_} -s -r compact)
endfunction ()

make_test (lowio-test)
