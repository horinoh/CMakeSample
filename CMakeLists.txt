cmake_minimum_required(VERSION 3.6.1)

# ソリューション
project(MyProject)

message("CMAKE_CXX_FLAGS = " ${CMAKE_CXX_FLAGS})
message("CMAKE_CXX_FLAGS_DEBUG = " ${CMAKE_CXX_FLAGS_DEBUG})
message("CMAKE_CXX_FLAGS_RELEASE = " ${CMAKE_CXX_FLAGS_RELEASE})

# パッケージ検索
# XXXConfig.cmake または xxx-config.cmake という名前のファイルを検索するので用意しておく
# Vulkan
find_package(VULKAN REQUIRED)
if(VULKAN_FOUND)
  include_directories(${VULKAN_INCLUDE_DIRS})
  link_directories(${VULKAN_LIBRARY_DIRS})
  add_definitions(${VULKAN_DEFINITIONS})

  message("VULKAN is FOUND")
  message(${VULKAN_INCLUDE_DIRS})
  message(${VULKAN_LIBRARY_DIRS})
  message(${VULKAN_LIBRARIES})
  message(${VULKAN_DEFINITIONS})
endif()

# (ソリューション内)プロジェクト
add_subdirectory(HelloWorld)
#add_subdirectory(Vulkan)

if(VULKAN_FOUND)
#  target_link_libraries(Vulkan ${VULKAN_LIBRARIES})
endif()

# スタートアッププロジェクト
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT HelloWorld)

