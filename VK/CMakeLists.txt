cmake_minimum_required(VERSION 3.6.1)

set(ProjectName VK)
project(${ProjectName})

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
file(GLOB_RECURSE ResourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.rc)
file(GLOB_RECURSE IconFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.ico)
add_executable(${ProjectName} ${SourceFiles} ${HeaderFiles} ${ResourceFiles} ${IconFiles})

# サブシステム (ウインドウアプリにする)
# __tmainCRTStartup が未解決なら恐らくコンソールアプリ
set_target_properties(${ProjectName} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")

# target_link_libaries() は add_executable() より下でやること
if(VULKAN_FOUND)
  target_link_libraries(${ProjectName} ${VULKAN_LIBRARIES})
endif()
