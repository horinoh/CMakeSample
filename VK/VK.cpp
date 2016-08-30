// VK.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "VK.h"

#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include <functional>

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#ifndef VERIFY_SUCCEEDED
#define VERIFY_SUCCEEDED(vr) if (VK_SUCCESS != (vr)) { assert(false); }
#endif

#ifndef GET_INSTANCE_PROC_ADDR
#define GET_INSTANCE_PROC_ADDR(instance, proc) reinterpret_cast<PFN_vk ## proc>(vkGetInstanceProcAddr(instance, "vk" #proc));
#endif
#ifndef GET_DEVICE_PROC_ADDR
#define GET_DEVICE_PROC_ADDR(device, proc) reinterpret_cast<PFN_vk ## proc>(vkGetDeviceProcAddr(device, "vk" #proc));
#endif

#ifndef SHADER_PATH
#ifdef _DEBUG
#define SHADER_PATH L".\\"
#else
#define SHADER_PATH L".\\"
#endif
#endif

#define DRAW_PLOYGON
#define USE_DEPTH_STENCIL

namespace VK {
	VkInstance Instance = VK_NULL_HANDLE;

#ifdef _DEBUG
	VkDebugReportCallbackEXT DebugReportCallback = VK_NULL_HANDLE;
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallback;
	PFN_vkDestroyDebugReportCallbackEXT vkDestoryDebugReportCallback;
	auto DebugReport = [](VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) -> VkBool32
	{
		if (VK_DEBUG_REPORT_ERROR_BIT_EXT & flags) {
			std::cout << pMessage << std::endl;
		}
		else if (VK_DEBUG_REPORT_WARNING_BIT_EXT & flags) {
			std::cout << pMessage << std::endl;
		}
		else if (VK_DEBUG_REPORT_INFORMATION_BIT_EXT & flags) {
			//std::cout << pMessage << std::endl;
		}
		else if (VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT & flags) {
			std::cout << pMessage << std::endl;
		}
		else if (VK_DEBUG_REPORT_DEBUG_BIT_EXT & flags) {
			//std::cout << pMessage << std::endl;
		}
		return false; //!< エラー時は基本 fakse を返してアボート
	};
#endif

	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceMemoryProperties PhysicalDeviceMemoryProperties;

	uint32_t GraphicsQueueFamilyIndex;
	VkDevice Device = VK_NULL_HANDLE;
	VkQueue Queue = VK_NULL_HANDLE;

	VkCommandPool CommandPool = VK_NULL_HANDLE;
	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;

	VkFence Fence = VK_NULL_HANDLE;
	VkSemaphore PresentSemaphore = VK_NULL_HANDLE;

	VkSurfaceKHR Surface = VK_NULL_HANDLE;
	VkExtent2D SurfaceExtent2D;
	VkFormat SurfaceFormat;
	VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> SwapchainImages;
	std::vector<VkImageView> SwapchainImageViews;
	uint32_t SwapchainImageIndex;

	VkFormat DepthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
	VkImage DepthStencilImage = VK_NULL_HANDLE;
	VkDeviceMemory DepthStencilDeviceMemory = VK_NULL_HANDLE;
	VkImageView DepthStencilImageView = VK_NULL_HANDLE;

	std::vector<VkShaderModule> ShaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> PipelineShaderStageCreateInfos;

	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;

	using Vertex = struct Vertex { glm::vec3 Positon; glm::vec4 Color; };
	std::vector<VkVertexInputBindingDescription> VertexInputBindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions;
	VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo;

	VkBuffer VertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory VertexDeviceMemory = VK_NULL_HANDLE;
	VkBuffer IndexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory IndexDeviceMemory = VK_NULL_HANDLE;
	uint32_t IndexCount;

	VkViewport Viewport;
	VkRect2D ScissorRect;

	VkRenderPass RenderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> Framebuffers;

	VkPipeline Pipeline = VK_NULL_HANDLE;

	const VkClearColorValue SkyBlue = { 0.529411793f, 0.807843208f, 0.921568692f, 1.0f };
	const std::vector<VkClearValue> ClearValues = {
		{
			{ SkyBlue },
			{ 1.0f, 0 }
		}
	};

	static VkAccessFlags GetSrcAccessMask(VkImageLayout Old, VkImageLayout New)
	{
		VkAccessFlags SrcAccessMask = 0;
		switch (Old) {
		case VK_IMAGE_LAYOUT_UNDEFINED: break;
		case VK_IMAGE_LAYOUT_PREINITIALIZED: SrcAccessMask = VK_ACCESS_HOST_WRITE_BIT; break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: SrcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: SrcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: SrcAccessMask = VK_ACCESS_TRANSFER_READ_BIT; break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: SrcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: SrcAccessMask = VK_ACCESS_SHADER_READ_BIT; break;
		}
		switch (New)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: SrcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT; break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: SrcAccessMask = VK_ACCESS_TRANSFER_READ_BIT; break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			if (0 == SrcAccessMask) {
				SrcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			break;
		}
		return SrcAccessMask;
	}
	static VkAccessFlags GetDstAccessMask(VkImageLayout New)
	{
		switch (New)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return VK_ACCESS_TRANSFER_WRITE_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return VK_ACCESS_TRANSFER_READ_BIT;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return VK_ACCESS_SHADER_READ_BIT;
		}
		return 0;
	}
	void SetImageLayout(VkCommandBuffer CommandBuffer, VkImage Image, VkImageLayout Old, VkImageLayout New)
	{
		const auto SrcAccessMask = GetSrcAccessMask(Old, New);
		const auto DstAccessMask = GetDstAccessMask(New);

		const VkImageSubresourceRange ImageSubresourceRange = {
			VK_IMAGE_ASPECT_COLOR_BIT,
			0, 1,
			0, 1,
		};
		const VkImageMemoryBarrier ImageMemoryBarrier = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			SrcAccessMask,
			DstAccessMask,
			Old,
			New,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			Image,
			ImageSubresourceRange
		};
		vkCmdPipelineBarrier(CommandBuffer,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &ImageMemoryBarrier);
	}
	uint32_t GetMemoryTypeIndex(uint32_t MemoryTypeBits, VkMemoryPropertyFlags MemoryPropertyFlags)
	{
		for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
			if ((MemoryTypeBits & 1) == 1) {
				if ((PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & MemoryPropertyFlags) == MemoryPropertyFlags) {
					return i;
				}
			}
			MemoryTypeBits >>= 1;
		}
		assert(false && "");
		return 0xffffffff;
	}
	VkFormat GetSupportedDepthFormat(VkPhysicalDevice PhysicalDevice)
	{
		const std::vector<VkFormat> DepthFormats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};
		for (auto& i : DepthFormats) {
			VkFormatProperties FormatProperties;
			vkGetPhysicalDeviceFormatProperties(PhysicalDevice, i, &FormatProperties);
			if (FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				return i;
			}
		}
		assert(false && "DepthFormat not found");
		return VK_FORMAT_UNDEFINED;
	}

	void ExecuteCommandBuffer()
	{
		VERIFY_SUCCEEDED(vkDeviceWaitIdle(Device));

		const VkPipelineStageFlags PipelineStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		const VkSubmitInfo SubmitInfo = {
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0, nullptr,
			&PipelineStageFlags,
			1, &CommandBuffer,
			0, nullptr
		};
		VERIFY_SUCCEEDED(vkQueueSubmit(Queue, 1, &SubmitInfo, Fence));
		VERIFY_SUCCEEDED(vkQueueWaitIdle(Queue));
	}
	void WaitForFence()
	{
		VkResult Result;
		const uint64_t TimeOut = 100000000;
		do {
			Result = vkWaitForFences(Device, 1, &Fence, VK_TRUE, TimeOut);
		} while (VK_SUCCESS != Result);
		VERIFY_SUCCEEDED(Result);

		VERIFY_SUCCEEDED(vkResetFences(Device, 1, &Fence));
	}

	void CreateInstance()
	{
		//!< インスタンスのレイヤ、エクステンションの列挙
		uint32_t LayerPropertyCount = 0;
		vkEnumerateInstanceLayerProperties(&LayerPropertyCount, nullptr);
		if (LayerPropertyCount) {
			std::vector<VkLayerProperties> LayerProperties(LayerPropertyCount);
			vkEnumerateInstanceLayerProperties(&LayerPropertyCount, LayerProperties.data());
			std::cout << "Layers And Extensions (Instance)" << std::endl;
			for (const auto& i : LayerProperties) {
				std::cout << "\t" << "\"" << i.layerName << "\"" << std::endl;
				uint32_t ExtensionPropertyCount = 0;
				vkEnumerateInstanceExtensionProperties(i.layerName, &ExtensionPropertyCount, nullptr);
				if (ExtensionPropertyCount) {
					std::vector<VkExtensionProperties> ExtensionProperties(ExtensionPropertyCount);
					vkEnumerateInstanceExtensionProperties(i.layerName, &ExtensionPropertyCount, ExtensionProperties.data());
					for (const auto& j : ExtensionProperties) {
						std::cout << "\t" << "\t" << "\"" << j.extensionName << "\"" << std::endl;
					}
				}

			}
		}
		//!< インスタンスの作成
		const VkApplicationInfo ApplicationInfo = {
			VK_STRUCTURE_TYPE_APPLICATION_INFO,
			nullptr,
			"VKDX App", 0,
			"VKDX Engine", 0,
			VK_API_VERSION_1_0
		};
		const std::vector<const char*> EnabledExtensions = {
			VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef _DEBUG
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
		};
		const std::vector<const char*> EnabledLayerNames = {
#ifdef _DEBUG
			"VK_LAYER_LUNARG_standard_validation",
#endif
		};
		const VkInstanceCreateInfo InstanceCreateInfo = {
			VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			nullptr,
			0,
			&ApplicationInfo,
			static_cast<uint32_t>(EnabledLayerNames.size()), EnabledLayerNames.data(),
			static_cast<uint32_t>(EnabledExtensions.size()), EnabledExtensions.data()
		};
		VERIFY_SUCCEEDED(vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance));
	}
#ifdef _DEBUG
	void CreateDebugReport()
	{
		vkCreateDebugReportCallback = GET_INSTANCE_PROC_ADDR(Instance, CreateDebugReportCallbackEXT);
		vkDestoryDebugReportCallback = GET_INSTANCE_PROC_ADDR(Instance, DestroyDebugReportCallbackEXT);

		//!< デバッグコールバックの登録
		const VkDebugReportCallbackCreateInfoEXT DebugReportCallbackCreateInfo = {
			VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
			nullptr,
			VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
			VK_DEBUG_REPORT_WARNING_BIT_EXT |
			VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
			VK_DEBUG_REPORT_ERROR_BIT_EXT |
			VK_DEBUG_REPORT_DEBUG_BIT_EXT |
			VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT,
			DebugReport,
			nullptr
		};
		VERIFY_SUCCEEDED(vkCreateDebugReportCallback(Instance, &DebugReportCallbackCreateInfo, nullptr, &DebugReportCallback));
	}
#endif
	void GetPhysicalDevice()
	{
		//!< 物理デバイスの列挙
		uint32_t PhysicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, nullptr);
		assert(PhysicalDeviceCount && "PhysicalDevice not found");
		std::vector<VkPhysicalDevice> PhysicalDevices(PhysicalDeviceCount);
		vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, PhysicalDevices.data());
		for (const auto& i : PhysicalDevices) {
			//!< デバイスプロパティ
			VkPhysicalDeviceProperties PhysicalDeviceProperties;
			vkGetPhysicalDeviceProperties(i, &PhysicalDeviceProperties);
			std::cout << "\"" << PhysicalDeviceProperties.deviceName << "\"" << std::endl;

			//!< デバイスメモリプロパティ	
			vkGetPhysicalDeviceMemoryProperties(i, &PhysicalDeviceMemoryProperties);

			//!< デバイスフィーチャー
			VkPhysicalDeviceFeatures PhysicalDeviceFeatures;
			vkGetPhysicalDeviceFeatures(i, &PhysicalDeviceFeatures);

			//!< 物理デバイスのレイヤ、エクステンションの列挙
			uint32_t LayerPropertyCount = 0;
			vkEnumerateDeviceLayerProperties(i, &LayerPropertyCount, nullptr);
			if (LayerPropertyCount) {
				std::vector<VkLayerProperties> LayerProperties(LayerPropertyCount);
				std::cout << "\t" << "Layers And Extensions (PhysicalDevice)" << std::endl;
				for (const auto& j : LayerProperties) {
					std::cout << "\t" << "\t" << "\"" << j.layerName << "\"" << std::endl;
					uint32_t ExtensionPropertyCount = 0;
					vkEnumerateDeviceExtensionProperties(i, j.layerName, &ExtensionPropertyCount, nullptr);
					if (ExtensionPropertyCount) {
						std::vector<VkExtensionProperties> ExtensionProperties(ExtensionPropertyCount);
						vkEnumerateInstanceExtensionProperties(j.layerName, &ExtensionPropertyCount, ExtensionProperties.data());
						for (const auto& k : ExtensionProperties) {
							std::cout << "\t" << "\t" << "\t" << "\"" << k.extensionName << "\"" << std::endl;

							if (0 == strcmp(k.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
								std::cout << "\t" << "\t" << "\t" << "MARKER FOUND" << std::endl;
							}
						}
					}
				}
			}
		}
		//!< 最初の物理デバイスを選択
		PhysicalDevice = PhysicalDevices[0];
	}
	void CreateDevice()
	{
		//!< キュープロパティの列挙
		uint32_t QueueFamilyPropertyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, nullptr);
		assert(QueueFamilyPropertyCount && "Queue Family not found");
		std::vector<VkQueueFamilyProperties> QueueProperties(QueueFamilyPropertyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, QueueProperties.data());
#ifdef _DEBUG
		std::cout << "QUEUE" << std::endl;
		for (const auto& i : QueueProperties) {
			std::cout << "\tQueueCount = " << i.queueCount << ", [";
			if (VK_QUEUE_GRAPHICS_BIT & i.queueFlags) { std::cout << " GRAPHICS "; }
			if (VK_QUEUE_COMPUTE_BIT & i.queueFlags) { std::cout << " COMPUTE "; }
			if (VK_QUEUE_TRANSFER_BIT & i.queueFlags) { std::cout << " TRANSFER "; }
			if (VK_QUEUE_SPARSE_BINDING_BIT & i.queueFlags) { std::cout << " SPARSE "; }
			std::cout << "]" << std::endl;
		}
#endif
		for (uint32_t i = 0; i < QueueFamilyPropertyCount; ++i) {
			if (VK_QUEUE_GRAPHICS_BIT & QueueProperties[i].queueFlags) {
				GraphicsQueueFamilyIndex = i;
			}
		}

		//!< デバイスの作成
		const std::vector<const char*> EnabledLayerNames = {
#ifdef _DEBUG
			"VK_LAYER_LUNARG_standard_validation",
#endif
		};
		const std::vector<const char*> EnabledExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};
		const std::vector<float> QueuePriorities = {
			0.0f
		};
		const std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos = {
			{
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				nullptr,
				0,
				GraphicsQueueFamilyIndex,
				static_cast<uint32_t>(QueuePriorities.size()), QueuePriorities.data()
			},
		};
		const VkDeviceCreateInfo DeviceCreateInfo = {
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(QueueCreateInfos.size()), QueueCreateInfos.data(),
			static_cast<uint32_t>(EnabledLayerNames.size()), EnabledLayerNames.data(),
			static_cast<uint32_t>(EnabledExtensions.size()), EnabledExtensions.data(),
			nullptr
		};
		VERIFY_SUCCEEDED(vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device));

		//!< キューの取得
		vkGetDeviceQueue(Device, GraphicsQueueFamilyIndex, 0, &Queue);
	}
	void CreateCommandBuffer()
	{
		//!< コマンドプールの作成
		const VkCommandPoolCreateInfo CommandPoolInfo = {
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			GraphicsQueueFamilyIndex
		};
		VERIFY_SUCCEEDED(vkCreateCommandPool(Device, &CommandPoolInfo, nullptr, &CommandPool));

		//!< コマンドバッファの作成
		const VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			CommandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1
		};
		VERIFY_SUCCEEDED(vkAllocateCommandBuffers(Device, &CommandBufferAllocateInfo, &CommandBuffer))
	}
	void CreateFence()
	{
		const VkFenceCreateInfo FenceCreateInfo = {
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			nullptr,
			0
		};
		VERIFY_SUCCEEDED(vkCreateFence(Device, &FenceCreateInfo, nullptr, &Fence));
	}
	void CreateSemaphore()
	{
		const VkSemaphoreCreateInfo SemaphoreCreateInfo = {
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			nullptr,
			0
		};
		VERIFY_SUCCEEDED(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &PresentSemaphore));
	}
	void CreateSwapchain(HINSTANCE hInst, HWND hWnd)
	{
		//!< サーフェスの作成
		const VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {
			VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			hInst,
			hWnd
		};
		VERIFY_SUCCEEDED(vkCreateWin32SurfaceKHR(Instance, &SurfaceCreateInfo, nullptr, &Surface));

		VkBool32 Supported;
		VERIFY_SUCCEEDED(vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, GraphicsQueueFamilyIndex, Surface, &Supported));
		assert(VK_TRUE == Supported && "Surface not supported");

		VkSurfaceCapabilitiesKHR SurfaceCapabilities;
		VERIFY_SUCCEEDED(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities));
		SurfaceExtent2D = SurfaceCapabilities.currentExtent;

		//!< サーフェスフォーマットの列挙
		uint32_t SurfaceFormatCount;
		VERIFY_SUCCEEDED(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &SurfaceFormatCount, nullptr));
		assert(SurfaceFormatCount && "SurfaceFormat not found");
		std::vector<VkSurfaceFormatKHR> SurfaceFormats(SurfaceFormatCount);
		VERIFY_SUCCEEDED(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &SurfaceFormatCount, SurfaceFormats.data()));
		SurfaceFormat = SurfaceFormats[0].format;

		//!< サーフェスプレゼントモードの列挙
		uint32_t SurfacePresentModeCount;
		VERIFY_SUCCEEDED(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &SurfacePresentModeCount, nullptr));
		assert(SurfacePresentModeCount);
		std::vector<VkPresentModeKHR> SurfacePresentModes(SurfacePresentModeCount);
		VERIFY_SUCCEEDED(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &SurfacePresentModeCount, SurfacePresentModes.data()));

		//!< スワップチェインの作成
		auto OldSwapchain = Swapchain;
		const VkSwapchainCreateInfoKHR SwapchainCreateInfo = {
			VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			nullptr,
			0,
			Surface,
			SurfaceCapabilities.minImageCount,
			SurfaceFormats[0].format,
			SurfaceFormats[0].colorSpace,
			SurfaceExtent2D,
			1,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0, nullptr,
			SurfaceCapabilities.currentTransform,
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			SurfacePresentModes[0],
			true,
			OldSwapchain
		};
		VERIFY_SUCCEEDED(vkCreateSwapchainKHR(Device, &SwapchainCreateInfo, nullptr, &Swapchain));
		if (VK_NULL_HANDLE != OldSwapchain) {
			vkDestroySwapchainKHR(Device, OldSwapchain, nullptr);
		}

		//!< スワップチェインイメージの取得
		uint32_t SwapchainImageCount;
		VERIFY_SUCCEEDED(vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImageCount, nullptr));
		SwapchainImages.resize(SwapchainImageCount);
		VERIFY_SUCCEEDED(vkGetSwapchainImagesKHR(Device, Swapchain, &SwapchainImageCount, SwapchainImages.data()));

		//!< スワップチェインイメージビューの作成
		SwapchainImageViews.resize(SwapchainImageCount);
		const VkComponentMapping ComponentMapping = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		const VkCommandBufferInheritanceInfo CommandBufferInheritanceInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			nullptr,
			VK_NULL_HANDLE,
			0,
			VK_NULL_HANDLE,
			VK_FALSE,
			0,
			0
		};
		const VkCommandBufferBeginInfo CommandBufferBeginInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			0,
			&CommandBufferInheritanceInfo
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo)); {
			for (uint32_t i = 0; i < SwapchainImageCount; ++i) {
				const VkImageSubresourceRange ImageSubresourceRange = {
					VK_IMAGE_ASPECT_COLOR_BIT,
					0, 1,
					0, 1
				};
				const VkImageViewCreateInfo ImageViewCreateInfo = {
					VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					nullptr,
					0,
					SwapchainImages[i],
					VK_IMAGE_VIEW_TYPE_2D,
					SurfaceFormats[0].format,
					ComponentMapping,
					ImageSubresourceRange
				};
				VERIFY_SUCCEEDED(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &SwapchainImageViews[i]));

				//!< 「使用前にメモリを塗りつぶせ」と怒られるので、初期カラーで塗りつぶす
				SetImageLayout(CommandBuffer, SwapchainImages[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); {
					const VkClearColorValue Green = { 0.0f, 1.0f, 0.0f, 1.0f };
					vkCmdClearColorImage(CommandBuffer, SwapchainImages[i], VK_IMAGE_LAYOUT_GENERAL, &Green, 1, &ImageSubresourceRange);
				} SetImageLayout(CommandBuffer, SwapchainImages[i], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			}
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CommandBuffer));

		ExecuteCommandBuffer();
		WaitForFence();

		VERIFY_SUCCEEDED(vkAcquireNextImageKHR(Device, Swapchain, UINT64_MAX, PresentSemaphore, nullptr, &SwapchainImageIndex));
	}
	void CreateDepthStencil()
	{
#ifdef USE_DEPTH_STENCIL
		DepthFormat = GetSupportedDepthFormat(PhysicalDevice);
		const VkExtent3D Extent3D = { SurfaceExtent2D.width, SurfaceExtent2D.height, 1 };
		const VkImageCreateInfo ImageCreateInfo = {
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			0,
			VK_IMAGE_TYPE_2D,
			DepthFormat,
			Extent3D,
			1,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0, nullptr,
			VK_IMAGE_LAYOUT_UNDEFINED
		};
		VERIFY_SUCCEEDED(vkCreateImage(Device, &ImageCreateInfo, nullptr, &DepthStencilImage));

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(Device, DepthStencilImage, &MemoryRequirements);
		const VkMemoryAllocateInfo MemoryAllocateInfo = {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			MemoryRequirements.size,
			GetMemoryTypeIndex(MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
		};
		VERIFY_SUCCEEDED(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &DepthStencilDeviceMemory));
		VERIFY_SUCCEEDED(vkBindImageMemory(Device, DepthStencilImage, DepthStencilDeviceMemory, 0));

		const VkComponentMapping ComponentMapping = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		const VkImageSubresourceRange ImageSubresourceRange = {
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			0, 1,
			0, 1
		};
		const VkImageViewCreateInfo ImageViewCreateInfo = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			DepthStencilImage,
			VK_IMAGE_VIEW_TYPE_2D,
			DepthFormat,
			ComponentMapping,
			ImageSubresourceRange
		};
		VERIFY_SUCCEEDED(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &DepthStencilImageView));
#endif
	}
	VkShaderModule CreateShaderModule(const std::wstring& Path)
	{
		VkShaderModule ShaderModule = VK_NULL_HANDLE;

		std::ifstream In(Path.c_str(), std::ios::in | std::ios::binary);
		assert(false == In.fail());

		In.seekg(0, std::ios_base::end);
		const auto CodeSize = In.tellg();
		In.seekg(0, std::ios_base::beg);

		auto Code = new char[CodeSize];
		In.read(Code, CodeSize);
		In.close();

		const VkShaderModuleCreateInfo ModuleCreateInfo = {
			VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			nullptr,
			0,
			CodeSize, reinterpret_cast<uint32_t*>(Code)
		};
		VERIFY_SUCCEEDED(vkCreateShaderModule(Device, &ModuleCreateInfo, nullptr, &ShaderModule));

		delete[] Code;

		assert(VK_NULL_HANDLE != ShaderModule);
		return ShaderModule;
	}
	void CreateShader()
	{
#ifdef DRAW_PLOYGON
		ShaderModules.push_back(CreateShaderModule(SHADER_PATH L"VS.vert.spv"));
		ShaderModules.push_back(CreateShaderModule(SHADER_PATH L"FS.frag.spv"));

		PipelineShaderStageCreateInfos = {
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				nullptr,
				0,
				VK_SHADER_STAGE_VERTEX_BIT, ShaderModules[0],
				"main",
				nullptr
			},
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				nullptr,
				0,
				VK_SHADER_STAGE_FRAGMENT_BIT, ShaderModules[1],
				"main",
				nullptr
			}
		};
#endif
	}
	void CreateDescriptorSet()
	{
#ifdef DRAW_PLOYGON
		const VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(DescriptorSetLayouts.size()), DescriptorSetLayouts.data(),
			0, nullptr
		};
		VERIFY_SUCCEEDED(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout));
#endif
	}
	void CreateVertexInput()
	{
#ifdef DRAW_PLOYGON
		Vertex Vertices;
		VertexInputBindingDescriptions = {
			{ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
		};
		VertexInputAttributeDescriptions = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12 }
		};
		PipelineVertexInputStateCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(VertexInputBindingDescriptions.size()), VertexInputBindingDescriptions.data(),
			static_cast<uint32_t>(VertexInputAttributeDescriptions.size()), VertexInputAttributeDescriptions.data()
		};
#endif
	}
	void CreateBuffer(const VkBufferUsageFlagBits Usage, VkBuffer* Buffer, VkDeviceMemory* DeviceMemory, const void *Source, const VkDeviceSize Size)
	{
		//!< ステージングバッファの作成
		const VkBufferCreateInfo StagingBufferCreateInfo = {
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			nullptr,
			0,
			Size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0, nullptr
		};
		VkBuffer StagingBuffer = VK_NULL_HANDLE;
		VERIFY_SUCCEEDED(vkCreateBuffer(Device, &StagingBufferCreateInfo, nullptr, &StagingBuffer));

		VkMemoryRequirements StagingMemoryRequirements;
		vkGetBufferMemoryRequirements(Device, StagingBuffer, &StagingMemoryRequirements);

		const VkMemoryAllocateInfo StagingMemoryAllocateInfo = {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			StagingMemoryRequirements.size,
			GetMemoryTypeIndex(StagingMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		};
		VkDeviceMemory StagingDeviceMemory = VK_NULL_HANDLE;
		VERIFY_SUCCEEDED(vkAllocateMemory(Device, &StagingMemoryAllocateInfo, nullptr, &StagingDeviceMemory));

		void* Data;
		VERIFY_SUCCEEDED(vkMapMemory(Device, StagingDeviceMemory, 0, StagingMemoryAllocateInfo.allocationSize, 0, &Data)); {
			memcpy(Data, Source, Size);
		} vkUnmapMemory(Device, StagingDeviceMemory);

		VERIFY_SUCCEEDED(vkBindBufferMemory(Device, StagingBuffer, StagingDeviceMemory, 0));

		//!< デバイスローカルバッファの作成
		const VkBufferCreateInfo BufferCreateInfo = {
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			nullptr,
			0,
			Size,
			static_cast<VkBufferUsageFlags>(Usage) | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0, nullptr
		};
		VERIFY_SUCCEEDED(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, Buffer));

		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(Device, *Buffer, &MemoryRequirements);

		const VkMemoryAllocateInfo MemoryAllocateInfo = {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			MemoryRequirements.size,
			GetMemoryTypeIndex(MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		};
		VERIFY_SUCCEEDED(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, DeviceMemory));

		VERIFY_SUCCEEDED(vkBindBufferMemory(Device, *Buffer, *DeviceMemory, 0));

		//!< ステージングバッファ → デバイスローカルバッファ コマンド発行
		const VkCommandBufferBeginInfo BeginInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			0,
			nullptr
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CommandBuffer, &BeginInfo)); {
			const VkBufferCopy BufferCopy = {
				0,
				0,
				Size
			};
			vkCmdCopyBuffer(CommandBuffer, StagingBuffer, *Buffer, 1, &BufferCopy);
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CommandBuffer));

		ExecuteCommandBuffer();
		WaitForFence();

		vkDestroyBuffer(Device, StagingBuffer, nullptr);
		vkFreeMemory(Device, StagingDeviceMemory, nullptr);
	}
	void CreateVertexBuffer()
	{
#ifdef DRAW_PLOYGON
		const std::vector<Vertex> Vertices = {
			{ { 0.0f, 0.5f, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.5f, -0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		};
		const auto Stride = sizeof(Vertices[0]);
		const auto Size = static_cast<VkDeviceSize>(Stride * Vertices.size());

		CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &VertexBuffer, &VertexDeviceMemory, Vertices.data(), Size);
#endif
	}
	void CreateIndexBuffer()
	{
#ifdef DRAW_PLOYGON
		const std::vector<uint32_t> Indices = { 0, 1, 2 };

		IndexCount = static_cast<uint32_t>(Indices.size());
		const auto Size = static_cast<VkDeviceSize>(sizeof(Indices[0]) * IndexCount);

		CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &IndexBuffer, &IndexDeviceMemory, Indices.data(), Size);
#endif
	}
	void CreateViewport()
	{
		Viewport = {
			0, 0,
			static_cast<float>(SurfaceExtent2D.width), static_cast<float>(SurfaceExtent2D.height),
			0.0f, 1.0f
		};
		ScissorRect = {
			{ 0, 0 },
			{ SurfaceExtent2D.width, SurfaceExtent2D.height }
		};
	}
	void CreateRenderPass()
	{
		const std::vector<VkAttachmentDescription> AttachmentDescriptions = {
			{
				0,
				SurfaceFormat,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			},
#ifdef USE_DEPTH_STENCIL
			{
				0,
				DepthFormat,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			},
#endif
		};
		const std::vector<VkAttachmentReference> ColorAttachmentReferences = {
			{
				0,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}
		};
#ifdef USE_DEPTH_STENCIL
		const VkAttachmentReference DepthAttachmentReferences = {
			1,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
#endif
		const std::vector<VkSubpassDescription> SubpassDescriptions = {
			{
				0,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				0, nullptr,
				static_cast<uint32_t>(ColorAttachmentReferences.size()), ColorAttachmentReferences.data(),
				nullptr,
#ifdef USE_DEPTH_STENCIL
				&DepthAttachmentReferences,
#else
				nullptr,
#endif
				0, nullptr
			}
		};
		const VkRenderPassCreateInfo RenderPassCreateInfo = {
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(AttachmentDescriptions.size()), AttachmentDescriptions.data(),
			static_cast<uint32_t>(SubpassDescriptions.size()), SubpassDescriptions.data(),
			0, nullptr
		};
		VERIFY_SUCCEEDED(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &RenderPass));
	}
	void CreateFramebuffer()
	{
		Framebuffers.resize(SwapchainImages.size());
		for (uint32_t i = 0; i < Framebuffers.size(); ++i) {
			const std::vector<VkImageView> Attachments = {
				SwapchainImageViews[i],
#ifdef USE_DEPTH_STENCIL
				DepthStencilImageView,
#endif
			};
			const VkFramebufferCreateInfo FramebufferCreateInfo = {
				VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				nullptr,
				0,
				RenderPass,
				static_cast<uint32_t>(Attachments.size()), Attachments.data(),
				SurfaceExtent2D.width, SurfaceExtent2D.height,
				1
			};
			VERIFY_SUCCEEDED(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &Framebuffers[i]));
		}
	}
	void CreatePipeline()
	{
#ifdef DRAW_PLOYGON
		const VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			VK_FALSE
		};

		const VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			nullptr,
			0,
			1, &Viewport,
			1, &ScissorRect
		};
		const VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_FALSE,
			VK_FALSE,
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_BACK_BIT,
			VK_FRONT_FACE_COUNTER_CLOCKWISE,
			VK_FALSE, 0.0f, 0.0f, 0.0f,
			1.0f
		};
		const VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_SAMPLE_COUNT_1_BIT,
			VK_FALSE, 0.0f,
			nullptr,
			VK_FALSE, VK_FALSE
		};

		const VkStencilOpState FrontStencilOpState = {
			VK_STENCIL_OP_KEEP,
			VK_STENCIL_OP_KEEP,
			VK_STENCIL_OP_KEEP,
			VK_COMPARE_OP_NEVER, 0, 0, 0
		};
		const VkStencilOpState BackStencilOpState = {
			VK_STENCIL_OP_KEEP,
			VK_STENCIL_OP_KEEP,
			VK_STENCIL_OP_KEEP,
			VK_COMPARE_OP_ALWAYS, 0, 0, 0
		};
		const VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS_OR_EQUAL,
			VK_FALSE,
			VK_FALSE,
			FrontStencilOpState,
			BackStencilOpState,
			0.0f, 0.0f
		};
		const std::vector<VkPipelineColorBlendAttachmentState> VkPipelineColorBlendAttachmentStates = {
			{
				VK_FALSE,
				VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
			}
		};
		const VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_FALSE, VK_LOGIC_OP_CLEAR,
			static_cast<uint32_t>(VkPipelineColorBlendAttachmentStates.size()), VkPipelineColorBlendAttachmentStates.data(),
			{ 0.0f, 0.0f, 0.0f, 0.0f }
		};
		const std::vector<VkDynamicState> DynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		const VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(DynamicStates.size()), DynamicStates.data()
		};
		const std::vector<VkGraphicsPipelineCreateInfo> GraphicsPipelineCreateInfos = {
			{
				VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
				nullptr,
				0,
				static_cast<uint32_t>(PipelineShaderStageCreateInfos.size()), PipelineShaderStageCreateInfos.data(),
				&PipelineVertexInputStateCreateInfo,
				&PipelineInputAssemblyStateCreateInfo,
				nullptr,
				&PipelineViewportStateCreateInfo,
				&PipelineRasterizationStateCreateInfo,
				&PipelineMultisampleStateCreateInfo,
				&PipelineDepthStencilStateCreateInfo,
				&PipelineColorBlendStateCreateInfo,
				&PipelineDynamicStateCreateInfo,
				PipelineLayout,
				RenderPass,
				0,
				VK_NULL_HANDLE, 0
			}
		};
		VERIFY_SUCCEEDED(vkCreateGraphicsPipelines(Device, nullptr, static_cast<uint32_t>(GraphicsPipelineCreateInfos.size()), GraphicsPipelineCreateInfos.data(), nullptr, &Pipeline));
#endif
	}

	void PopulateCommandBuffer()
	{
		VERIFY_SUCCEEDED(vkResetCommandPool(Device, CommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));

		const VkCommandBufferBeginInfo BeginInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			0,
			nullptr
		};
		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CommandBuffer, &BeginInfo)); {
			vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);
			vkCmdSetScissor(CommandBuffer, 0, 1, &ScissorRect);
			SetImageLayout(CommandBuffer, SwapchainImages[SwapchainImageIndex], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL); {
				const VkRenderPassBeginInfo RenderPassBeginInfo = {
					VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					nullptr,
					RenderPass,
					Framebuffers[SwapchainImageIndex],
					{
						0, 0,
						SurfaceExtent2D.width, SurfaceExtent2D.height
					},
					static_cast<uint32_t>(ClearValues.size()), ClearValues.data()
				};
				vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE); {
#ifdef DRAW_PLOYGON
					vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

					VkDeviceSize Offsets = 0;
					vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertexBuffer, &Offsets);
					vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

					vkCmdDrawIndexed(CommandBuffer, IndexCount, 1, 0, 0, 0);
#endif
				} vkCmdEndRenderPass(CommandBuffer);
			} SetImageLayout(CommandBuffer, SwapchainImages[SwapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CommandBuffer));
	}
	void Present()
	{
		const VkPresentInfoKHR PresentInfo = {
			VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			nullptr,
			1, &PresentSemaphore,
			1, &Swapchain,
			&SwapchainImageIndex,
			nullptr
		};
		VERIFY_SUCCEEDED(vkQueuePresentKHR(Queue, &PresentInfo));

		VERIFY_SUCCEEDED(vkAcquireNextImageKHR(Device, Swapchain, UINT64_MAX, PresentSemaphore, nullptr, &SwapchainImageIndex));
	}

	void OnCreate(HINSTANCE hInst, HWND hWnd)
	{
		CreateInstance();
#ifdef _DEBUG
		CreateDebugReport();
#endif
		GetPhysicalDevice();
		CreateDevice();
		CreateCommandBuffer();
		CreateFence();
		CreateSemaphore();
		CreateSwapchain(hInst, hWnd);
		CreateDepthStencil();
		CreateShader();
		CreateDescriptorSet();
		CreateVertexInput();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateRenderPass();
		CreateFramebuffer();
		CreateViewport();
		CreatePipeline();
	}
	void OnPaint(HWND hWhd)
	{
		PopulateCommandBuffer();
		ExecuteCommandBuffer();
		WaitForFence();

		Present();
	}
	void OnDestroy()
	{
		if (VK_NULL_HANDLE != Pipeline) {
			vkDestroyPipeline(Device, Pipeline, nullptr);
		}
		for (auto i : Framebuffers) {
			vkDestroyFramebuffer(Device, i, nullptr);
		}
		if (VK_NULL_HANDLE != RenderPass) {
			vkDestroyRenderPass(Device, RenderPass, nullptr);
		}
		if (VK_NULL_HANDLE != IndexDeviceMemory) {
			vkFreeMemory(Device, IndexDeviceMemory, nullptr);
		}
		if (VK_NULL_HANDLE != IndexBuffer) {
			vkDestroyBuffer(Device, IndexBuffer, nullptr);
		}
		if (VK_NULL_HANDLE != VertexDeviceMemory) {
			vkFreeMemory(Device, VertexDeviceMemory, nullptr);
		}
		if (VK_NULL_HANDLE != VertexBuffer) {
			vkDestroyBuffer(Device, VertexBuffer, nullptr);
		}
		if (VK_NULL_HANDLE != PipelineLayout) {
			vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
		}
		for (auto i : DescriptorSetLayouts) {
			vkDestroyDescriptorSetLayout(Device, i, nullptr);
		}
		for (auto i : ShaderModules) {
			vkDestroyShaderModule(Device, i, nullptr);
		}
		if (VK_NULL_HANDLE != DepthStencilImage) {
			vkDestroyImage(Device, DepthStencilImage, nullptr);
		}
		if (VK_NULL_HANDLE != DepthStencilImageView) {
			vkDestroyImageView(Device, DepthStencilImageView, nullptr);
		}
		if (VK_NULL_HANDLE != DepthStencilDeviceMemory) {
			vkFreeMemory(Device, DepthStencilDeviceMemory, nullptr);
		}
		for (auto i : SwapchainImageViews) {
			vkDestroyImageView(Device, i, nullptr);
		}
		if (VK_NULL_HANDLE != Swapchain) {
			vkDestroySwapchainKHR(Device, Swapchain, nullptr);
		}
		if (VK_NULL_HANDLE != Surface) {
			vkDestroySurfaceKHR(Instance, Surface, nullptr);
		}
		if (VK_NULL_HANDLE != PresentSemaphore) {
			vkDestroySemaphore(Device, PresentSemaphore, nullptr);
		}
		if (VK_NULL_HANDLE != Fence) {
			vkDestroyFence(Device, Fence, nullptr);
		}
		if (VK_NULL_HANDLE != CommandBuffer) {
			vkFreeCommandBuffers(Device, CommandPool, 1, &CommandBuffer);
		}
		if (VK_NULL_HANDLE != CommandPool) {
			vkDestroyCommandPool(Device, CommandPool, nullptr);
		}
		if (VK_NULL_HANDLE != Device) {
			vkDestroyDevice(Device, nullptr);
		}
#ifdef _DEBUG
		if (VK_NULL_HANDLE != vkDestoryDebugReportCallback) {
			vkDestoryDebugReportCallback(Instance, DebugReportCallback, nullptr);
		}
#endif
		if (VK_NULL_HANDLE != Instance) {
			vkDestroyInstance(Instance, nullptr);
		}
	}
};

#define MAX_LOADSTRING 100

// グローバル変数:
HINSTANCE hInst;								// 現在のインターフェイス
TCHAR szTitle[MAX_LOADSTRING];					// タイトル バーのテキスト
TCHAR szWindowClass[MAX_LOADSTRING];			// メイン ウィンドウ クラス名

												// このコード モジュールに含まれる関数の宣言を転送します:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: ここにコードを挿入してください。
	MSG msg;
	HACCEL hAccelTable;

	// グローバル文字列を初期化しています。
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_VK, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// アプリケーションの初期化を実行します:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VK));

	// メイン メッセージ ループ:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VK));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_VK);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	auto a = GetLastError();
	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND	- アプリケーション メニューの処理
//  WM_PAINT	- メイン ウィンドウの描画
//  WM_DESTROY	- 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 選択されたメニューの解析:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_CREATE:
		VK::OnCreate(hInst, hWnd);
		SetTimer(hWnd, NULL, 1000 / 60, nullptr);
		break;
	case WM_TIMER:
		SendMessage(hWnd, WM_PAINT, 0, 0);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		VK::OnPaint(hWnd);
		// TODO: 描画コードをここに追加してください...
		EndPaint(hWnd, &ps);
		break;
	case WM_KEYDOWN:
		if (VK_ESCAPE == wParam) {
			SendMessage(hWnd, WM_DESTROY, 0, 0);
		}
		break;
	case WM_DESTROY:
		VK::OnDestroy();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
