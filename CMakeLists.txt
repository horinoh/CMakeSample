# File - Delete cache してから 「Visual Studio のバージョンと x64 等を選択して」 Generate する

cmake_minimum_required(VERSION 3.6.1)

# ソリューション
project(MyProject)

#message("CMAKE_CXX_FLAGS = " ${CMAKE_CXX_FLAGS})
#message("CMAKE_CXX_FLAGS_DEBUG = " ${CMAKE_CXX_FLAGS_DEBUG})
#message("CMAKE_CXX_FLAGS_RELEASE = " ${CMAKE_CXX_FLAGS_RELEASE})

# 32bit, 64bit 判断
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64bit
else()
endif()

# VSバージョン判断
# 1600 VS10
# 1700 VS11
# 1800 VS12(2013)
if(MSVC_VERSION GREATER 1800)
  # VS14(2015)
else()
endif()

# Debug 以外のビルド構成 XXX をデバッグビルド扱いにする場合
# (target_link_libraries() での debug, optimized 選択)
#set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS "Debug;XXX")

# Vulkan プロジェクト
add_subdirectory(VK)

# スタートアッププロジェクト設定
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VK)
