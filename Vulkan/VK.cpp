#include <iostream>
#include <vector>
#include <cassert>

#include "VK.h"

int main(int argc, char* argv[])
{
    using namespace std;
    
    cout << "Hello World" << endl;

	uint32_t InstanceLayerPropertyCount = 0;
	vkEnumerateInstanceLayerProperties(&InstanceLayerPropertyCount, nullptr);
	if (InstanceLayerPropertyCount) {
		std::vector<VkLayerProperties> LayerProperties(InstanceLayerPropertyCount);
		vkEnumerateInstanceLayerProperties(&InstanceLayerPropertyCount, LayerProperties.data());
		for (const auto& i : LayerProperties) {
			i.layerName;
		}
	}

#if 0
	VkInstance Instance = VK_NULL_HANDLE;
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
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
	};
	const std::vector<const char*> EnabledLayerNames = {
#ifdef _DEBUG
		"VK_LAYER_LUNARG_standard_validation"
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
	vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance);

	uint32_t InstanceExtensionPropertyCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &InstanceExtensionPropertyCount, nullptr);
	if (InstanceExtensionPropertyCount) {
		std::vector<VkExtensionProperties> ExtensionProperties(InstanceExtensionPropertyCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &InstanceExtensionPropertyCount, ExtensionProperties.data());
		for (const auto& i : ExtensionProperties) {
			i.extensionName;
		}
	}

	if (VK_NULL_HANDLE != Instance) {
		uint32_t PhysicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, nullptr);
		assert(PhysicalDeviceCount && "PhysicalDevice not found");
		std::vector<VkPhysicalDevice> PhysicalDevices(PhysicalDeviceCount);
		vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, PhysicalDevices.data());
		for (const auto& i : PhysicalDevices) {
			VkPhysicalDeviceProperties PhysicalDeviceProperties;
			vkGetPhysicalDeviceProperties(i, &PhysicalDeviceProperties);

			VkPhysicalDeviceFeatures PhysicalDeviceFeatures;
			vkGetPhysicalDeviceFeatures(i, &PhysicalDeviceFeatures);
		}

		vkDestroyInstance(Instance, nullptr);
	}
#endif
}