cmake_minimum_required(VERSION 3.6.1)

set(ProjectName VK)
project(${ProjectName})

# パッケージ検索
# XXXConfig.cmake または xxx-config.cmake という名前のファイルを検索する
# VULKAN
set(VULKAN_DIR .)
find_package(VULKAN REQUIRED)
if(VULKAN_FOUND)
    include_directories(${VULKAN_INCLUDE_DIRS})
    link_directories(${VULKAN_LIBRARY_DIRS})
    link_libraries(${VULKAN_LIBRARIES})
    add_definitions(${VULKAN_DEFINITIONS})
else()
    message("VULKAN package not found")
    
    include_directories($ENV{VK_SDK_PATH}\\Include)
    link_directories($ENV{VK_SDK_PATH}\\Lib)
    link_libraries(vulkan-1)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
endif()

# GLM
set(GLM_DIR .)
find_package(GLM REQUIRED)
if(GLM_FOUND)
    include_directories(${GLM_INCLUDE_DIRS})
else()
    message("GLM package not found")
    
    include_directories($ENV{GLM_SDK_PATH})
endif()

# GLI
set(GLI_DIR .)
find_package(GLI REQUIRED)
if(GLI_FOUND)
    include_directories(${GLI_INCLUDE_DIRS})
else()
    message("GLI package not found")
    
    include_directories($ENV{GLI_SDK_PATH})
endif()

# Windows ビルド用に予め用意したファイル
# ReadMe.txt
# Resource.h
# small.ico
# stdafx.cpp, stdafx.h
# targetver.h
# VK.ico, VK.rc
file(GLOB_RECURSE SourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE HeaderFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE ResourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.rc)
file(GLOB_RECURSE IconFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.ico)
add_executable(${ProjectName} ${SourceFiles} ${HeaderFiles} ${ResourceFiles} ${IconFiles})

# サブシステム (ウインドウアプリにする)
# __tmainCRTStartup が未解決なら恐らくコンソールアプリ
set_target_properties(${ProjectName} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")

# target_link_libaries() は add_executable() より下でやること
#if(VULKAN_FOUND)
#  target_link_libraries(${ProjectName} ${VULKAN_LIBRARIES})
#endif()
