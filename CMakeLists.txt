cmake_minimum_required(VERSION 3.0)

# set project name from current directory
get_filename_component(BASENAME ${CMAKE_CURRENT_LIST_DIR} NAME)
project(${BASENAME})

#set (CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall  -march=native -std=c++17")

# define executable to build
include_directories("inc")
aux_source_directory(src SRC_FILES)
add_library(v4l2cpp ${SRC_FILES})

add_executable(test test.cpp)
target_link_libraries(test v4l2cpp pthread  /usr/local/lib/aarch64-linux-gnu/liblog4cplus.so )
