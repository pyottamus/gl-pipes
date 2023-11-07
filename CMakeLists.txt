cmake_minimum_required(VERSION 3.10)

# set the project name
project(gl_pipes)

add_subdirectory(src)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT gl_pipes)
# add the executable
#add_executable(gl_pipes main.cpp)