cmake_minimum_required(VERSION 3.10)
project(jmap C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

include(GNUInstallDirs)

set(LIB_SOURCES
    third_party/murmur3-master/murmur3.c
    src/jmap.c
    src/jmap_presets/jmap_int.c
    src/jmap_presets/jmap_string.c
)

add_library(jmap STATIC ${LIB_SOURCES})
set_target_properties(jmap PROPERTIES OUTPUT_NAME "jmap")

add_library(jmap_shared SHARED ${LIB_SOURCES})
set_target_properties(jmap_shared PROPERTIES OUTPUT_NAME "jmap")

target_include_directories(jmap PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(jmap_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS jmap jmap_shared
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .a
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .so
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install jmap.h dans /usr/local/include
install(FILES inc/jmap.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_custom_target(lib
    COMMAND ${CMAKE_COMMAND} --build . --target install
    DEPENDS jmap jmap_shared
)

add_executable(jmap_test src/main.c)
target_link_libraries(jmap_test jmap)
target_include_directories(jmap_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(test
    COMMAND ${CMAKE_COMMAND} --build . --target jmap_test
    DEPENDS jmap_test
)
