include_directories(.)

add_executable(main main.c recursive_reachability.c)
target_link_libraries(main sylvan popt)

add_executable(bddmc bddmc.c getrss.h getrss.c)
target_link_libraries(bddmc sylvan popt)
