
cmake_minimum_required (VERSION 3.6)

include (FindBoost)

set (BOOST_ROOT C:/tools/boost)
set (Boost_USE_STATIC_LIBS YES)
find_package (Boost REQUIRED COMPONENTS filesystem)

set (SOURCE_FILES
    openclose.cpp
    handle.cpp
    output.cpp
    fixture.cpp
    fixture.hpp
    main.cpp)

set_source_files_properties (main.cpp PROPERTIES COTIRE_EXCLUDED ON)

include_directories (${LOWIO_SOURCE_DIR}/ext)

add_executable (lowio-test ${SOURCE_FILES})
    target_link_libraries (lowio-test PRIVATE lowio Boost::disable_autolinking Boost::filesystem)

set_target_properties (lowio-test PROPERTIES COTIRE_PREFIX_HEADER_INCLUDE_PATH "${LOWIO_SOURCE_DIR}/ext")

if (COMMAND cotire)
    cotire (lowio-test)
endif ()

add_test (NAME lowio-test
          COMMAND test)
