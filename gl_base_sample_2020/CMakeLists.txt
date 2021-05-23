cmake_minimum_required(VERSION 3.5)
project(gl_base_sample_2020)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(SOURCE_FILES
        common.h
        glad.c
        main.cpp
        
        
        ShaderProgram.h
        ShaderProgram.cpp)

set(ADDITIONAL_INCLUDE_DIRS
        dependencies/include/GLAD)
set(ADDITIONAL_LIBRARY_DIRS
        dependencies/lib)   
set(ADDITIONAL_RUNTIME_LIBRARY_DIRS
        dependencies/bin)   

if(WIN32)
  set(ADDITIONAL_INCLUDE_DIRS 
        ${ADDITIONAL_INCLUDE_DIRS}
        dependencies/include)
  link_directories(${ADDITIONAL_LIBRARY_DIRS})
else()
	find_package(glfw3 REQUIRED)
endif()

include_directories(${ADDITIONAL_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)

add_executable(gl_base_sample_2020 ${SOURCE_FILES} "shader.h" "camera.h")

target_include_directories(gl_base_sample_2020 PRIVATE ${OPENGL_INCLUDE_DIR})


if(WIN32)
  add_custom_command(TARGET gl_base_sample_2020 POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/dependencies/bin" $<TARGET_FILE_DIR:gl_base_sample_2020>)
  set_target_properties(gl_base_sample_2020 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  target_compile_options(gl_base_sample_2020 PRIVATE)
  target_link_libraries(gl_base_sample_2020 LINK_PUBLIC ${OPENGL_gl_LIBRARY} glfw3dll)
else()
  target_compile_options(gl_base_sample_2020 PRIVATE -Wnarrowing)
  target_link_libraries(gl_base_sample_2020 LINK_PUBLIC ${OPENGL_gl_LIBRARY} glfw rt dl)
endif()

