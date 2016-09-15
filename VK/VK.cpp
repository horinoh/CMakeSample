// VK.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "VK.h"

#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include <functional>
#include <algorithm>

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
	VkSurfaceKHR Surface = VK_NULL_HANDLE;
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
#ifdef _DEBUG
	PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag = VK_NULL_HANDLE;
	PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert = VK_NULL_HANDLE;
	void SetObjectTag(VkDevice Device, VkDebugReportObjectTypeEXT DebugReportObjectType, uint64_t Object, const uint64_t TagName, const size_t TagSize, const void* Tag)
	{
		if (VK_NULL_HANDLE != vkDebugMarkerSetObjectTag) {
			VkDebugMarkerObjectTagInfoEXT DebugMarkerObjectTagInfo = {
				VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT,
				nullptr,
				DebugReportObjectType,
				Object,
				TagName,
				TagSize,
				Tag
			};
			VERIFY_SUCCEEDED(vkDebugMarkerSetObjectTag(Device, &DebugMarkerObjectTagInfo));
		}
	}
	void SetObjectName(VkDevice Device, VkDebugReportObjectTypeEXT DebugReportObjectType, uint64_t Object, const char* ObjectName)
	{
		if (VK_NULL_HANDLE != vkDebugMarkerSetObjectName) {
			VkDebugMarkerObjectNameInfoEXT DebugMarkerObjectNameInfo = {
				VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT,
				nullptr,
				DebugReportObjectType,
				Object,
				ObjectName
			};
			VERIFY_SUCCEEDED(vkDebugMarkerSetObjectName(Device, &DebugMarkerObjectNameInfo));
		}
	}
	void BeginMarkerRegion(VkCommandBuffer CommandBuffer, const char* MarkerName, glm::vec4 Color)
	{
		if (VK_NULL_HANDLE != vkCmdDebugMarkerBegin) {
			VkDebugMarkerMarkerInfoEXT DebugMarkerMarkerInfo = {
				VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,
				nullptr,
				MarkerName,
			};
			memcpy(&DebugMarkerMarkerInfo.color, &Color, sizeof(DebugMarkerMarkerInfo.color));
			vkCmdDebugMarkerBegin(CommandBuffer, &DebugMarkerMarkerInfo);
		}
	}
	void EndMarkerRegion(VkCommandBuffer CommandBuffer)
	{
		if (VK_NULL_HANDLE != vkCmdDebugMarkerEnd) {
			vkCmdDebugMarkerEnd(CommandBuffer);
		}
	}
	void InsertMarker(VkCommandBuffer CommandBuffer, const char* MarkerName, glm::vec4 Color)
	{
		if (VK_NULL_HANDLE != vkCmdDebugMarkerInsert) {
			VkDebugMarkerMarkerInfoEXT DebugMarkerMarkerInfo = {
				VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,
				nullptr,
				MarkerName,
			};
			memcpy(&DebugMarkerMarkerInfo.color, &Color, sizeof(DebugMarkerMarkerInfo.color));
			vkCmdDebugMarkerInsert(CommandBuffer, &DebugMarkerMarkerInfo);
		}
	}
#endif
	uint32_t GraphicsQueueFamilyIndex = UINT32_MAX;
	uint32_t PresentQueueFamilyIndex = UINT32_MAX;
	VkDevice Device = VK_NULL_HANDLE;
	VkQueue GraphicsQueue = VK_NULL_HANDLE;
	VkQueue PresentQueue = VK_NULL_HANDLE;

	VkCommandPool CommandPool = VK_NULL_HANDLE;
	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;

	VkFence Fence = VK_NULL_HANDLE;
	VkSemaphore NextImageAcquiredSemaphore = VK_NULL_HANDLE;	//!< 次イメージ取得完了(プレゼント完了)セマフォ (描画が可能になった) 
	VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;		//!< 描画完了セマフォ (プレゼントが可能になった)

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

	using Vertex = struct Vertex { glm::vec3 Position; glm::vec4 Color; };

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
	bool HasDebugMarker()
	{
		uint32_t LayerPropertyCount = 0;
		vkEnumerateDeviceLayerProperties(PhysicalDevice, &LayerPropertyCount, nullptr);
		if (LayerPropertyCount) {
			std::vector<VkLayerProperties> LayerProperties(LayerPropertyCount);
			vkEnumerateDeviceLayerProperties(PhysicalDevice, &LayerPropertyCount, LayerProperties.data());
			for (const auto& i : LayerProperties) {
				uint32_t ExtensionPropertyCount = 0;
				vkEnumerateDeviceExtensionProperties(PhysicalDevice, i.layerName, &ExtensionPropertyCount, nullptr);
				if (ExtensionPropertyCount) {
					std::vector<VkExtensionProperties> ExtensionProperties(ExtensionPropertyCount);
					vkEnumerateDeviceExtensionProperties(PhysicalDevice, i.layerName, &ExtensionPropertyCount, ExtensionProperties.data());
					for (const auto& j : ExtensionProperties) {
						if (!strcmp(j.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
							return true;
						}
					}
				}
			}
		}
		return false;
	};
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
#ifdef VK_USE_PLATFORM_WIN32_KHR
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
	void CreateSurface(HINSTANCE hInst, HWND hWnd)
	{
		//!< サーフェスの作成
#ifdef VK_USE_PLATFORM_WIN32_KHR
		const VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {
			VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			hInst,
			hWnd
		};
		VERIFY_SUCCEEDED(vkCreateWin32SurfaceKHR(Instance, &SurfaceCreateInfo, nullptr, &Surface));
#endif
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
						}
					}
				}
			}
		}

		//!< ここでは物理デバイスを選択 #TODO
		PhysicalDevice = PhysicalDevices[0];
		//!< デバイスメモリプロパティ	
		vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &PhysicalDeviceMemoryProperties);
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
			//!< グラフィックをサポートするキュー
			if (VK_QUEUE_GRAPHICS_BIT & QueueProperties[i].queueFlags) {
				if (UINT32_MAX == GraphicsQueueFamilyIndex) {
					GraphicsQueueFamilyIndex = i;
				}
			}
			//!< プレゼントをサポートするキュー
			VkBool32 Supported;
			VERIFY_SUCCEEDED(vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &Supported));
			if (Supported) {
				if (UINT32_MAX == PresentQueueFamilyIndex) {
					PresentQueueFamilyIndex = i;
				}
			}
		}
		//!< グラフィックとプレゼントを同時にサポートするキューがあれば優先
		if (GraphicsQueueFamilyIndex != PresentQueueFamilyIndex) {
			for (uint32_t i = 0; i < QueueFamilyPropertyCount; ++i) {
				if (VK_QUEUE_GRAPHICS_BIT & QueueProperties[i].queueFlags) {
					VkBool32 Supported;
					VERIFY_SUCCEEDED(vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &Supported));
					if (Supported) {
						GraphicsQueueFamilyIndex = i;
						PresentQueueFamilyIndex = i;
						break;
					}
				}
			}
		}

		//!< デバイスの作成
		const std::vector<const char*> EnabledLayerNames = {
#ifdef _DEBUG
			"VK_LAYER_LUNARG_standard_validation",
#endif
		};
		std::vector<const char*> EnabledExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};
		//!< 可能ならマーカ拡張を有効
		if (HasDebugMarker()) {
			EnabledExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
		}
		const std::vector<float> QueuePriorities = {
			0.0f
		};

		std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos = {
			{
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				nullptr,
				0,
				GraphicsQueueFamilyIndex,
				static_cast<uint32_t>(QueuePriorities.size()), QueuePriorities.data()
			},
		};
		//!< グラフィックとプレゼントのキューインデックスが別の場合は追加で必要
		if (GraphicsQueueFamilyIndex != PresentQueueFamilyIndex) {
			QueueCreateInfos.push_back(
			{
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				nullptr,
				0,
				PresentQueueFamilyIndex,
				static_cast<uint32_t>(QueuePriorities.size()), QueuePriorities.data()
			}
			);
		}
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

		//!< キューの取得 (グラフィック、プレゼントキューは同じものの場合もあるが別名で取得)
		vkGetDeviceQueue(Device, GraphicsQueueFamilyIndex, 0, &GraphicsQueue);
		vkGetDeviceQueue(Device, PresentQueueFamilyIndex, 0, &PresentQueue);
	}
#ifdef _DEBUG
	void CreateDebugMarker()
	{
		vkDebugMarkerSetObjectTag = GET_DEVICE_PROC_ADDR(Device, DebugMarkerSetObjectTagEXT);
		vkDebugMarkerSetObjectName = GET_DEVICE_PROC_ADDR(Device, DebugMarkerSetObjectNameEXT);
		vkCmdDebugMarkerBegin = GET_DEVICE_PROC_ADDR(Device, CmdDebugMarkerBeginEXT);
		vkCmdDebugMarkerEnd = GET_DEVICE_PROC_ADDR(Device, CmdDebugMarkerEndEXT);
		vkCmdDebugMarkerInsert = GET_DEVICE_PROC_ADDR(Device, CmdDebugMarkerInsertEXT);
	}
#endif
	void CreateCommandBuffer()
	{
		//!< コマンドプールの作成
		const VkCommandPoolCreateInfo CommandPoolInfo = {
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, //!< VK_COMMAND_POOL_CREATE_TRANSIENT_BIT 短期間にリセットや解放される場合に指定する
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
			VK_FENCE_CREATE_SIGNALED_BIT //!< 初回と２回目以降を同じに扱う為に、シグナル済みで作成
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
		VERIFY_SUCCEEDED(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &NextImageAcquiredSemaphore));
		VERIFY_SUCCEEDED(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &RenderFinishedSemaphore));
	}
	void CreateSwapchain()
	{
		VkSurfaceCapabilitiesKHR SurfaceCapabilities;
		VERIFY_SUCCEEDED(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities));
		//!< イメージサイズ (-1 の場合は選択可能)
		if (-1 == SurfaceCapabilities.currentExtent.width) {
			SurfaceExtent2D = { (std::max)((std::min<uint32_t>)(1280, SurfaceCapabilities.maxImageExtent.width), SurfaceCapabilities.minImageExtent.width), (std::max)((std::min<uint32_t>)(720, SurfaceCapabilities.maxImageExtent.height), SurfaceCapabilities.minImageExtent.height) };
		}
		else {
			SurfaceExtent2D = SurfaceCapabilities.currentExtent;
		}

		//!< イメージ枚数 (ここでは1枚多く取る ... MAILBOX の場合3枚あった方が良いので)
		const uint32_t MinImageCount = (std::min)(SurfaceCapabilities.minImageCount + 1, SurfaceCapabilities.maxImageCount);

		//!< イメージ使用法 (可能ならVK_IMAGE_USAGE_TRANSFER_DST_BIT をセットする。イメージクリア用)
		const VkImageUsageFlags ImageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | (VK_IMAGE_USAGE_TRANSFER_DST_BIT & SurfaceCapabilities.supportedUsageFlags);

		//!< トランスフォーム (可能ならVK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		const VkSurfaceTransformFlagBitsKHR SurfaceTransformFlag = (VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR & SurfaceCapabilities.supportedTransforms) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : SurfaceCapabilities.currentTransform;

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
		/**
		可能なら VK_PRESENT_MODE_MAILBOX_KHR を選択
		VK_PRESENT_MODE_IMMEDIATE_KHR
		VK_PRESENT_MODE_MAILBOX_KHR ... Vブランクで表示される、テアリングは起こらない、キューは1つで常に最新で上書きされる (ゲーム)
		VK_PRESENT_MODE_FIFO_KHR ... Vブランクで表示される、テアリングは起こらない (ムービー)
		VK_PRESENT_MODE_FIFO_RELAXED_KHR ... 1Vブランク以上経ったイメージは次のVブランクを待たずにリリースされ得る、テアリングが起こる
		*/
		const VkPresentModeKHR PresentMode = [&]() {
			for (auto i : SurfacePresentModes) {
				if (VK_PRESENT_MODE_MAILBOX_KHR == i) {
					return i;
				}
			}
			for (auto i : SurfacePresentModes) {
				if (VK_PRESENT_MODE_FIFO_KHR == i) {
					return i;
				}
			}
			return SurfacePresentModes[0];
		}();

		//!< スワップチェインの作成
		auto OldSwapchain = Swapchain;
		const VkSwapchainCreateInfoKHR SwapchainCreateInfo = {
			VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			nullptr,
			0,
			Surface,
			MinImageCount,
			SurfaceFormat,
			SurfaceFormats[0].colorSpace,
			SurfaceExtent2D,
			1,
			ImageUsageFlags,
			VK_SHARING_MODE_EXCLUSIVE,
			0, nullptr,
			SurfaceTransformFlag,
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			PresentMode,
			VK_TRUE,
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
		//!< スワップチェインイメージはプレゼント前迄に VK_IMAGE_LAYOUT_PRESENT_SRC_KHR にすること

		//!< スワップチェインイメージビューの作成
		SwapchainImageViews.resize(SwapchainImageCount);
		const VkComponentMapping ComponentMapping = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
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
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
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

				//!< イメージをカラーで初期化、イメージレイアウトの変更
				const VkImageMemoryBarrier ImageMemoryBarrier_PresentToTransfer = {
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					0,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, //!<「現在のレイアウト」or「UNDEFINED」を指定すること、イメージコンテンツを保持したい場合は「UNDEFINED」はダメ
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					PresentQueueFamilyIndex,
					PresentQueueFamilyIndex,
					SwapchainImages[i],
					ImageSubresourceRange
				};
				const VkImageMemoryBarrier ImageMemoryBarrier_TransferToPresent = {
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_MEMORY_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					PresentQueueFamilyIndex,
					PresentQueueFamilyIndex,
					SwapchainImages[i],
					ImageSubresourceRange
				};
				vkCmdPipelineBarrier(CommandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &ImageMemoryBarrier_PresentToTransfer); {

					//!< 初期カラーで塗りつぶす
					const VkClearColorValue Green = { 0.0f, 1.0f, 0.0f, 1.0f };
					vkCmdClearColorImage(CommandBuffer, SwapchainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &Green, 1, &ImageSubresourceRange);

				} vkCmdPipelineBarrier(CommandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &ImageMemoryBarrier_TransferToPresent);
			}
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CommandBuffer));

		VERIFY_SUCCEEDED(vkDeviceWaitIdle(Device));
		const VkPipelineStageFlags PipelineStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		const VkSubmitInfo SubmitInfo = {
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0, nullptr, &PipelineStageFlags,
			1, &CommandBuffer,
			0, nullptr
		};
		VERIFY_SUCCEEDED(vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, Fence));
		VERIFY_SUCCEEDED(vkQueueWaitIdle(GraphicsQueue));

		WaitForFence();

		//!< 次のイメージが取得できたらセマフォが通知される
		VERIFY_SUCCEEDED(vkAcquireNextImageKHR(Device, Swapchain, UINT64_MAX, NextImageAcquiredSemaphore, VK_NULL_HANDLE, &SwapchainImageIndex));
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
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
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
	/**
	#TODO 本当は、まとまったサイズで vkAllocateMemory() 確保し、vkBindBufferMemory(..., Offset) のオフセットにより複数のバッファを管理したほうが効率が良い
	*/
	void CreateBuffer(const VkBufferUsageFlagBits Usage, const VkAccessFlags AccessFlag, const VkPipelineStageFlags PipelineStageFlag, VkBuffer* Buffer, VkDeviceMemory* DeviceMemory, const void *Source, const VkDeviceSize Size)
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

			//!< VK_MEMORY_PROPERTY_HOST_COHERENT_BIT でない場合に必要
			const VkMappedMemoryRange MappedMemoryRange = {
				VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				nullptr,
				StagingDeviceMemory,
				0,
				VK_WHOLE_SIZE
			};
			VERIFY_SUCCEEDED(vkFlushMappedMemoryRanges(Device, 1, &MappedMemoryRange));

		} vkUnmapMemory(Device, StagingDeviceMemory); //!< vkUnmapMemory() はしなくても良い(パフォーマンスへの影響も無い)

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

			//!< バッファを「転送先として」から「目的のバッファとして」へ変更する
			const VkBufferMemoryBarrier BufferMemoryBarrier = {
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_MEMORY_WRITE_BIT,
				AccessFlag,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				*Buffer,
				0,
				VK_WHOLE_SIZE
			};
			vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, PipelineStageFlag, 0, 0, nullptr, 1, &BufferMemoryBarrier, 0, nullptr);

		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CommandBuffer));

		//!< サブミット
		const VkSubmitInfo SubmitInfo = {
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0, nullptr,
			nullptr,
			1, &CommandBuffer,
			0, nullptr
		};
		VERIFY_SUCCEEDED(vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE));
		VERIFY_SUCCEEDED(vkDeviceWaitIdle(Device)); //!< フェンスでも良いがここでは vkDeviceWaitIdle() で待つ

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

		CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, &VertexBuffer, &VertexDeviceMemory, Vertices.data(), Size);

#ifdef _DEBUG
		SetObjectName(Device, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, reinterpret_cast<uint64_t>(VertexBuffer), "vertex buffer for triangle");
#endif
#endif
	}
	void CreateIndexBuffer()
	{
#ifdef DRAW_PLOYGON
		const std::vector<uint32_t> Indices = { 0, 1, 2 };

		IndexCount = static_cast<uint32_t>(Indices.size());
		const auto Size = static_cast<VkDeviceSize>(sizeof(Indices[0]) * IndexCount);

		CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_ACCESS_INDEX_READ_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, &IndexBuffer, &IndexDeviceMemory, Indices.data(), Size);

#ifdef _DEBUG
		SetObjectName(Device, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, reinterpret_cast<uint64_t>(IndexBuffer), "index buffer for triangle");
#endif
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
				VK_ATTACHMENT_LOAD_OP_CLEAR, //!< レンダーパスの開始時にクリアを行う
				VK_ATTACHMENT_STORE_OP_STORE, //!< レンダーパス終了時に保存する(表示するのに必要)
				VK_ATTACHMENT_LOAD_OP_DONT_CARE, //!< (ステンシルは)カラーアタッチメントの場合は関係ない
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,//!< レンダーパス開始時のレイアウト (メモリバリアなしにサブパス間でレイアウトが変更される)
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR //!< レンダーパス終了時のレイアウト
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
			{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		};
#ifdef USE_DEPTH_STENCIL
		const VkAttachmentReference DepthAttachmentReferences = {
			1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
#endif
		const std::vector<VkSubpassDescription> SubpassDescriptions = {
			{
				0,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				0, nullptr,
				static_cast<uint32_t>(ColorAttachmentReferences.size()), ColorAttachmentReferences.data(), nullptr,
#ifdef USE_DEPTH_STENCIL
				&DepthAttachmentReferences,
#else
				nullptr,
#endif
				0, nullptr //!< このサブパスでは使用しないが、後のサブパスで使用する場合に指定しなくてはならない
			}
		};
		//!< イメージバリアより効率が良い
		const std::vector<VkSubpassDependency> SubpassDependencies = {
			{
				//!< サブパス
				VK_SUBPASS_EXTERNAL,							//!< サブパス外から
				0,												//!< サブパス0へ
																//!< ステージ
																VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,			//!< パイプラインの最後
																VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,	//!< カラー出力
																												//!< アクセス
																												VK_ACCESS_MEMORY_READ_BIT,						//!< リード
																												VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			//!< ライト
																												VK_DEPENDENCY_BY_REGION_BIT,
			},
			{
				0,
				VK_SUBPASS_EXTERNAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_MEMORY_READ_BIT,
				VK_DEPENDENCY_BY_REGION_BIT,
			}
		};
		const VkRenderPassCreateInfo RenderPassCreateInfo = {
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(AttachmentDescriptions.size()), AttachmentDescriptions.data(),
			static_cast<uint32_t>(SubpassDescriptions.size()), SubpassDescriptions.data(),
			static_cast<uint32_t>(SubpassDependencies.size()), SubpassDependencies.data()
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
	void CreateShader(std::vector<VkShaderModule>& ShaderModules, std::vector<VkPipelineShaderStageCreateInfo>& PipelineShaderStageCreateInfos)
	{
		ShaderModules = {
			CreateShaderModule(SHADER_PATH L"VS.vert.spv"),
			CreateShaderModule(SHADER_PATH L"FS.frag.spv")
		};
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
	}
	template<typename T>
	void CreateVertexInput(std::vector<VkVertexInputBindingDescription>& VertexInputBindingDescriptions, std::vector<VkVertexInputAttributeDescription>& VertexInputAttributeDescriptions, const uint32_t Binding = 0) {}
	template<>
	void CreateVertexInput<Vertex>(std::vector<VkVertexInputBindingDescription>& VertexInputBindingDescriptions, std::vector<VkVertexInputAttributeDescription>& VertexInputAttributeDescriptions, const uint32_t Binding)
	{
		VertexInputBindingDescriptions = {
			{ Binding, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
		};
		VertexInputAttributeDescriptions = {
			{ 0, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Position) },
			{ 1, Binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Color) }
		};
	}
	template<typename T>
	void CreatePipeline()
	{
#ifdef DRAW_PLOYGON
		std::vector<VkShaderModule> ShaderModules;
		std::vector<VkPipelineShaderStageCreateInfo> PipelineShaderStageCreateInfos;
		CreateShader(ShaderModules, PipelineShaderStageCreateInfos);

		std::vector<VkVertexInputBindingDescription> VertexInputBindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> VertexInputAttributeDescriptions;
		CreateVertexInput<T>(VertexInputBindingDescriptions, VertexInputAttributeDescriptions);
		const VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(VertexInputBindingDescriptions.size()), VertexInputBindingDescriptions.data(),
			static_cast<uint32_t>(VertexInputAttributeDescriptions.size()), VertexInputAttributeDescriptions.data()
		};

		const VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			VK_FALSE
		};

		//!< DynamicState にする場合は nullptr を指定できる、ただし個数は 0 にできないので注意。DynamicStateにした項目はコマンドから必ずセットすること
		const VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			nullptr,
			0,
			1, nullptr,
			1, nullptr
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

		//!< アタッチメント毎に異なるブレンドをしたい場合はデバイスで有効になっていないとだめ (VkPhysicalDeviceFeatures.independentBlend)
		const std::vector<VkPipelineColorBlendAttachmentState> PipelineColorBlendAttachmentStates = {
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
			static_cast<uint32_t>(PipelineColorBlendAttachmentStates.size()), PipelineColorBlendAttachmentStates.data(),
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

		//const VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {
		//};
		//VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
		//VERIFY_SUCCEEDED(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &DescriptorSetLayout));
		const VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			0, nullptr, //1, &DescriptorSetLayout,
			0, nullptr
		};
		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
		VERIFY_SUCCEEDED(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout));
		//if (VK_NULL_HANDLE != DescriptorSetLayout) {
		//	vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr);
		//	DescriptorSetLayout = VK_NULL_HANDLE;
		//}

		/**
		@note
		basePipelineHandle と basePipelineIndex は同時に使用できない(排他)
		親には VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT フラグが必要
		・basePipelineHandle ... 既に存在する場合、親パイプラインを指定
		・basePipelineIndex ... GraphicsPipelineCreateInfos 配列で親パイプラインも同時に作成する場合、配列内での親パイプラインの添字。親の添字の方が若い値でないといけない。
		*/
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
				VK_NULL_HANDLE, //!< VkPipeline basePipelineHandle;
				0				//!< int32_t basePipelineIndex;
			}
		};
		VERIFY_SUCCEEDED(vkCreateGraphicsPipelines(Device, nullptr, static_cast<uint32_t>(GraphicsPipelineCreateInfos.size()), GraphicsPipelineCreateInfos.data(), nullptr, &Pipeline));

		for (auto i : ShaderModules) {
			vkDestroyShaderModule(Device, i, nullptr);
		}
		if (VK_NULL_HANDLE != PipelineLayout) {
			vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
			PipelineLayout = VK_NULL_HANDLE;
		}
#endif
	}

	void PopulateCommandBuffer()
	{
		VERIFY_SUCCEEDED(vkResetCommandPool(Device, CommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));

		const VkCommandBufferBeginInfo BeginInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
#if 1
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
#else
			VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, //!< 前回のサブミットが完了していなくても、再度サブミットされ得る
#endif
			nullptr
		};

		VERIFY_SUCCEEDED(vkBeginCommandBuffer(CommandBuffer, &BeginInfo)); {
#ifdef _DEBUG
			BeginMarkerRegion(CommandBuffer, "begining of command buffer", glm::vec4(1, 0, 0, 1));
#endif
			vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);
			vkCmdSetScissor(CommandBuffer, 0, 1, &ScissorRect);
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

#ifdef _DEBUG
				InsertMarker(CommandBuffer, "draw polygon", glm::vec4(1, 1, 0, 1));
#endif
				vkCmdDrawIndexed(CommandBuffer, IndexCount, 1, 0, 0, 0);
#endif
			} vkCmdEndRenderPass(CommandBuffer);
#ifdef _DEBUG
			EndMarkerRegion(CommandBuffer);
#endif
		} VERIFY_SUCCEEDED(vkEndCommandBuffer(CommandBuffer));
	}
	void Present()
	{
		const VkPresentInfoKHR PresentInfo = {
			VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			nullptr,
			1, &RenderFinishedSemaphore,	//!< 描画が完了するまで待つ
			1, &Swapchain, &SwapchainImageIndex,
			nullptr
		};
		VERIFY_SUCCEEDED(vkQueuePresentKHR(GraphicsQueue, &PresentInfo));

		//!< 次のイメージが取得できたらセマフォが通知される
		VERIFY_SUCCEEDED(vkAcquireNextImageKHR(Device, Swapchain, UINT64_MAX, NextImageAcquiredSemaphore, VK_NULL_HANDLE, &SwapchainImageIndex));
	}

	void OnCreate(HINSTANCE hInst, HWND hWnd)
	{
		CreateInstance();
		CreateSurface(hInst, hWnd);
#ifdef _DEBUG
		CreateDebugReport();
#endif
		GetPhysicalDevice();
		CreateDevice();
#ifdef _DEBUG
		CreateDebugMarker();
#endif
		CreateCommandBuffer();
		CreateFence();
		CreateSemaphore();
		CreateSwapchain();
		CreateDepthStencil();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateRenderPass();
		CreateFramebuffer();
		CreateViewport();
		CreatePipeline<Vertex>();
	}
	void OnSize(HINSTANCE hInst, HWND hWnd)
	{
		if (VK_NULL_HANDLE != Device) {
			VERIFY_SUCCEEDED(vkDeviceWaitIdle(Device));
		}
	}
	void OnPaint(HWND hWhd)
	{
		PopulateCommandBuffer();

		VERIFY_SUCCEEDED(vkDeviceWaitIdle(Device));
		const VkPipelineStageFlags PipelineStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT; //VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		const VkSubmitInfo SubmitInfo = {
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			1, &NextImageAcquiredSemaphore, &PipelineStageFlags,	//!< 次イメージが取得できる(プレゼント完了)までウエイト
			1, &CommandBuffer,
			1, &RenderFinishedSemaphore								//!< 描画完了を通知する
		};
		VERIFY_SUCCEEDED(vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, Fence));
		VERIFY_SUCCEEDED(vkQueueWaitIdle(GraphicsQueue));

		WaitForFence();

		Present();
	}
	void OnDestroy()
	{
		if (VK_NULL_HANDLE != Device) {
			VERIFY_SUCCEEDED(vkDeviceWaitIdle(Device));
		}

		if (VK_NULL_HANDLE != Pipeline) {
			vkDestroyPipeline(Device, Pipeline, nullptr);
			Pipeline = VK_NULL_HANDLE;
		}
		for (auto i : Framebuffers) {
			vkDestroyFramebuffer(Device, i, nullptr);
		}
		Framebuffers.clear();
		if (VK_NULL_HANDLE != RenderPass) {
			vkDestroyRenderPass(Device, RenderPass, nullptr);
			RenderPass = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != IndexDeviceMemory) {
			vkFreeMemory(Device, IndexDeviceMemory, nullptr);
			IndexDeviceMemory = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != IndexBuffer) {
			vkDestroyBuffer(Device, IndexBuffer, nullptr);
			IndexBuffer = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != VertexDeviceMemory) {
			vkFreeMemory(Device, VertexDeviceMemory, nullptr);
			VertexDeviceMemory = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != VertexBuffer) {
			vkDestroyBuffer(Device, VertexBuffer, nullptr);
			VertexBuffer = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != DepthStencilImage) {
			vkDestroyImage(Device, DepthStencilImage, nullptr);
			DepthStencilImage = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != DepthStencilImageView) {
			vkDestroyImageView(Device, DepthStencilImageView, nullptr);
			DepthStencilImageView = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != DepthStencilDeviceMemory) {
			vkFreeMemory(Device, DepthStencilDeviceMemory, nullptr);
			DepthStencilDeviceMemory = VK_NULL_HANDLE;
		}
		for (auto i : SwapchainImageViews) {
			vkDestroyImageView(Device, i, nullptr);
		}
		SwapchainImageViews.clear();
		if (VK_NULL_HANDLE != Swapchain) {
			vkDestroySwapchainKHR(Device, Swapchain, nullptr);
			Swapchain = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != RenderFinishedSemaphore) {
			vkDestroySemaphore(Device, RenderFinishedSemaphore, nullptr);
			RenderFinishedSemaphore = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != NextImageAcquiredSemaphore) {
			vkDestroySemaphore(Device, NextImageAcquiredSemaphore, nullptr);
			RenderFinishedSemaphore = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != Fence) {
			vkDestroyFence(Device, Fence, nullptr);
			Fence = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != CommandBuffer) {
			vkFreeCommandBuffers(Device, CommandPool, 1, &CommandBuffer);
			CommandBuffer = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != CommandPool) {
			vkDestroyCommandPool(Device, CommandPool, nullptr);
			CommandPool = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != Device) {
			vkDestroyDevice(Device, nullptr);
			Device = VK_NULL_HANDLE;
		}
#ifdef _DEBUG
		if (VK_NULL_HANDLE != vkDestoryDebugReportCallback) {
			vkDestoryDebugReportCallback(Instance, DebugReportCallback, nullptr);
			vkDestoryDebugReportCallback = VK_NULL_HANDLE;
		}
#endif
		if (VK_NULL_HANDLE != Surface) {
			vkDestroySurfaceKHR(Instance, Surface, nullptr);
			Surface = VK_NULL_HANDLE;
		}
		if (VK_NULL_HANDLE != Instance) {
			vkDestroyInstance(Instance, nullptr);
			Instance = VK_NULL_HANDLE;
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
	case WM_SIZE:
		VK::OnSize(hInst, hWnd);
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
