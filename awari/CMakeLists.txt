include_directories(.)

add_executable(awari awari.cpp bitvec.cpp bitvec.h bvec_sylvan.cpp bvec_sylvan.h)
target_link_libraries(awari sylvan)
set_property(TARGET awari PROPERTY C_STANDARD 11)


