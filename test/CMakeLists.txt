
cmake_minimum_required (VERSION 3.6)

include (FindBoost)

set (BOOST_ROOT C:/tools/boost)
if ("${MSVC}")
    set (Boost_USE_STATIC_LIBS YES)
else ()
    set (Boost_USE_STATIC_LIBS NO)
endif ()

find_package (Boost REQUIRED COMPONENTS filesystem)

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

add_compile_options (-std=gnu++14)
set_source_files_properties (main.cpp PROPERTIES COTIRE_EXCLUDED ON)

include_directories (${LOWIO_SOURCE_DIR}/ext)

add_executable (lowio-test pch.cpp ${SOURCE_FILES})
    target_link_libraries (lowio-test PRIVATE lowio Boost::disable_autolinking Boost::filesystem)

set_target_properties (lowio-test PROPERTIES COTIRE_PREFIX_HEADER_INCLUDE_PATH "${LOWIO_SOURCE_DIR}/ext")

if (COMMAND cotire)
    set_target_properties (lowio-test PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "common.hpp")
    cotire (lowio-test)
endif ()

add_test (NAME lowio-test
          COMMAND test)
