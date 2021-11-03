cmake_minimum_required(VERSION 3.1)

# use included version of Sylvan, not installed version
include_directories(. ../sylvan/src/)

add_executable(bddmc bddmc.c getrss.h getrss.c)
target_link_libraries(bddmc ${CMAKE_SOURCE_DIR}/../sylvan/build/src/libsylvan.so)

add_executable(lddmc lddmc.c getrss.h getrss.c)
target_link_libraries(lddmc ${CMAKE_SOURCE_DIR}/../sylvan/build/src/libsylvan.so)
