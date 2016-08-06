cmake_minimum_required(VERSION 3.6.1)

# ソリューション
project(MyProject)

if(MSVC)
  message("MSVC")
else()
  message("NOT MSVC")
endif()
message("CMAKE_CXX_FLAGS = " ${CMAKE_CXX_FLAGS})
message("CMAKE_CXX_FLAGS_DEBUG = " ${CMAKE_CXX_FLAGS_DEBUG})
message("CMAKE_CXX_FLAGS_RELEASE = " ${CMAKE_CXX_FLAGS_RELEASE})

# (ソリューション内)プロジェクト
add_subdirectory(HelloWorld)
#add_subdirectory(XXX)

# スタートアッププロジェクト
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT HelloWorld)

# インクルードパス
#include_directories(${XXX_DIR}/include)

# ライブラリパス
#link_directories(${XXX_DIR}/lib)

# プリプロセッサ
#add_definitions(-DXXX -DYYY)

# ソースファイル
#set(Sources XXX.cpp)
#add_executable(MyProject ${Sources})

# ライブラリ
#target_link_libraries(MyProject XXX)

# パッケージ検索
# XXXConfig.cmake または xxx-config.cmake という名前のファイルを検索するので用意しておく
find_package(XXX REQUIRED)
if(XXX_FOUND)
#  インクルードパス
#  include_directories(${XXX_INCLUDE_DIRS})
#  ライブラリパス
#  link_directories(${XXX_LIBRARY_DIRS})
#  ライブラリ
#  target_link_libraries(HelloWorld ${XXX_LIBRARIES})
#  プリプロセッサ
#  add_definitions(${XXX_DEFINITIONS})

  message("XXX is FOUND")
  message(${XXX_INCLUDE_DIRS})
  message(${XXX_LIBRARY_DIRS})
  message(${XXX_LIBRARIES})
  message(${XXX_DEFINITIONS})
endif()
