cmake_minimum_required(VERSION 3.1)

include_directories(.)

add_executable(awari awari.cpp bitvec.cpp bitvec.h bvec_sylvan.cpp bvec_sylvan.h)
target_link_libraries(awari "/usr/local/lib/libsylvan.so") # do idea why this doesn't automatically find this with just "sylvan"
set_property(TARGET awari PROPERTY C_STANDARD 11)


