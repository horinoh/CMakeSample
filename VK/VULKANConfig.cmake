set(VULKAN_INCLUDE_DIRS $ENV{VK_SDK_PATH}/Include)
#set(VULKAN_LIBRARY_DIRS $ENV{VK_SDK_PATH}/Bin) # Version < 1.0.42.2
set(VULKAN_LIBRARY_DIRS $ENV{VK_SDK_PATH}/Lib) # Version >= 1.0.42.2
set(VULKAN_LIBRARIES debug vulkan-1 optimized vulkan-1)
if(MSVC)
    set(VULKAN_DEFINITIONS -DVK_USE_PLATFORM_WIN32_KHR)
endif()

mark_as_advanced(VULKAN)