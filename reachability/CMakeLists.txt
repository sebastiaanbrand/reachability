include_directories(.)

add_executable(bddmc bddmc.c getrss.h getrss.c)
target_link_libraries(bddmc sylvan)
