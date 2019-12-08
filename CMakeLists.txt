cmake_minimum_required(VERSION 3.15)
project(ps_lsof)

set(CMAKE_CXX_STANDARD 14)

add_executable(ps ps-lsof-implementation/own_ps.cpp ps-lsof-implementation/proc_stat_parse.cpp)
add_executable(lsof ps-lsof-implementation/lsof.cpp ps-lsof-implementation/proc_stat_parse.cpp)
add_executable(ext2_driver ext2/list_by_inode.c)