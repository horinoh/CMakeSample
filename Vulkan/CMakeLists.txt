cmake_minimum_required(VERSION 3.6.1)

file(GLOB_RECURSE SourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file (GLOB_RECURSE HeaderFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(Vulkan ${SourceFiles} ${HeaderFiles})