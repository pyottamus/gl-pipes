set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

include_directories(${GLEW_INCLUDE_DIRS})

add_executable(gl_pipes)
add_custom_command(
        TARGET gl_pipes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory  
                ${CMAKE_CURRENT_SOURCE_DIR}/../assets
                ${CMAKE_CURRENT_BINARY_DIR}  )
target_sources(gl_pipes PRIVATE main.cpp pyo_rawobj.hpp pyoUtils.hpp world.hpp pyo_stddef.h)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/common src/commons)
	

target_link_libraries(gl_pipes ${OPENGL_gl_LIBRARY} glfw GLEW::GLEW)
