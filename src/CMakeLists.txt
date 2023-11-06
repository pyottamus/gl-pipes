set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_subdirectory(deps/glfw)
add_subdirectory(deps/glm)

add_subdirectory(deps/glad)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)


add_executable(gl_pipes)


add_custom_command(
        TARGET gl_pipes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory  
                ${CMAKE_CURRENT_SOURCE_DIR}/../assets
                ${CMAKE_CURRENT_BINARY_DIR}  )
target_sources(gl_pipes PRIVATE main.cpp pyo_rawobj.hpp pyoUtils.hpp world.hpp pyo_stddef.h gl_classes.hpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/common src/commons)
	

target_link_libraries(gl_pipes ${OPENGL_gl_LIBRARY} glfw glad)
