cmake_minimum_required(VERSION 3.1)

project(ReachabilityAlgorithms)

set(CMAKE_BUILD_TYPE Release)

# use included version of Sylvan, not installed version
include_directories(. ../sylvan/src/)

add_executable(bddmc bddmc.c getrss.h getrss.c)
target_link_libraries(bddmc ${CMAKE_SOURCE_DIR}/../sylvan/build/src/libsylvan.so)

add_executable(lddmc lddmc.c ldd_custom.h ldd_custom.c getrss.h getrss.c)
target_link_libraries(lddmc ${CMAKE_SOURCE_DIR}/../sylvan/build/src/libsylvan.so)

add_executable(test_ldd_custom test_ldd_custom.c ldd_custom.h ldd_custom.c)
target_link_libraries(test_ldd_custom ${CMAKE_SOURCE_DIR}/../sylvan/build/src/libsylvan.so)

