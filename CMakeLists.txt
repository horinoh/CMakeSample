cmake_minimum_required(VERSION 3.6.1)

# ソリューション
project(MyProject)

#message("CMAKE_CXX_FLAGS = " ${CMAKE_CXX_FLAGS})
#message("CMAKE_CXX_FLAGS_DEBUG = " ${CMAKE_CXX_FLAGS_DEBUG})
#message("CMAKE_CXX_FLAGS_RELEASE = " ${CMAKE_CXX_FLAGS_RELEASE})

# HelloWorld プロジェクト
add_subdirectory(HelloWorld)
# Vulkan プロジェクト
add_subdirectory(Vulkan)

# スタートアッププロジェクト設定
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Vulkan)
