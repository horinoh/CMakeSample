cmake_minimum_required(VERSION 3.6.1)
project(Vulkan)

# パッケージ検索
# XXXConfig.cmake または xxx-config.cmake という名前のファイルを検索する
set(VULKAN_DIR .)
find_package(VULKAN REQUIRED)
if(VULKAN_FOUND)
    include_directories(${VULKAN_INCLUDE_DIRS})
    link_directories(${VULKAN_LIBRARY_DIRS})
    add_definitions(${VULKAN_DEFINITIONS})
endif()

file(GLOB_RECURSE SourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE HeaderFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
add_executable(Vulkan ${SourceFiles} ${HeaderFiles})

# target_link_libaries() は add_executable() より下でやること
if(VULKAN_FOUND)
  target_link_libraries(Vulkan ${VULKAN_LIBRARIES})
endif()
