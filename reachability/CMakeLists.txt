include_directories(.)

add_executable(bddmc bddmc.c getrss.h getrss.c)
target_link_libraries(bddmc sylvan)

add_executable(lddmc lddmc.c getrss.h getrss.c)
target_link_libraries(lddmc sylvan)
