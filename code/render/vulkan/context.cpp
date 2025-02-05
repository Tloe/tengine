#include "context.h"
#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "ds_string.h"
#include "vulkan.h"
#include "vulkan/vulkan.h"
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace {
  const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
  const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (const char *layerName : validation_layers) {
      bool layerFound = false;

      for (const auto &layerProperties : availableLayers) {
        if (strcmp(layerName, layerProperties.layerName) == 0) {
          layerFound = true;
          break;
        }
      }

      if (!layerFound) {
        return false;
      }
    }

    return true;
  }

  VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {

    fprintf(stderr, "validation layer:\n%s\n", pCallbackData->pMessage);

    return VK_FALSE;
  }

  VkDebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo() {
    VkDebugUtilsMessengerCreateInfoEXT create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debugCallback;

    return create_info;
  }

  void setupDebugMessenger(vulkan::Context in) {
    auto debug_create_info = populateDebugMessengerCreateInfo();

    auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(in.instance, "vkCreateDebugUtilsMessengerEXT"));

    VkResult result;
    if (fn != nullptr) {
      result = fn(in.instance, &debug_create_info, nullptr, &in.debugMessenger);
    } else {
      result = VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    if (result != VK_SUCCESS) {
      printf("failed to setup debug messenger!");
      exit(0);
    }
  }

  bool check_device_extension_support(mem::Arena *scratch, VkPhysicalDevice device) {
    auto available_extensions = vulkan::available_extensions(scratch, device);

    for (U32 i = 0; i < sizeof(device_extensions) / sizeof(device_extensions[0]); i++) {
      if (ds::array::contains(available_extensions,
                              ds::string::init(scratch, device_extensions[i]))) {
        return false;
      }
    }
    return true;
  }

  bool isDeviceSuitable(mem::Arena *scratch, VkPhysicalDevice device, VkSurfaceKHR surface) {
    // TODO : Base device suitability checks at:
    // https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families

    U32 graphics_family, presentFamily;
    if (!vulkan::queue_family_indices(device, surface, &graphics_family, &presentFamily)) {
      return false;
    }

    if (check_device_extension_support(scratch, device)) {
      return false;
    }

    auto swapChainSupport = vulkan::query_swap_chain_support(scratch, device, surface);
    if (swapChainSupport.formats._size == 0 || swapChainSupport.presentModes._size == 0) {
      return false;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return supportedFeatures.samplerAnisotropy;
  }

  VkPhysicalDevice
  pickSuitableDevice(mem::Arena *scratch, VkInstance instance, VkSurfaceKHR surface) {

    U32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0) {
      printf("failed to find GPUs with Vulkan support!");
      exit(0);
    }

    /* auto devices = ds::array::init<VkPhysicalDevice>(scratch, deviceCount, deviceCount); */
    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    for (int i = 0; i < device_count; i++) {
      if (isDeviceSuitable(scratch, devices[i], surface)) {
        return devices[i];
      }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
      printf("failed to find a suitable GPU");
      exit(0);
    }

    return VK_NULL_HANDLE;
  }
}

vulkan::Context vulkan::context::create(mem::Arena *scratch, SDL_Window *window , bool debug) {
  if (debug && !checkValidationLayerSupport()) {
    printf("validation layers requested, but not available!");
    exit(0);
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto required_extensions = vulkan::required_extension(scratch, debug);
  createInfo.enabledExtensionCount = required_extensions._size;
  createInfo.ppEnabledExtensionNames = required_extensions._data;

  if (debug) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(sizeof(validation_layers) / sizeof(validation_layers[0]));
    createInfo.ppEnabledLayerNames = validation_layers;

    auto debug_create_info = populateDebugMessengerCreateInfo();
    createInfo.pNext = &debug_create_info;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  printf("required extensions:\n");
  for (int i = 0; i < required_extensions._size; i++) {
    printf("\t%s\n", required_extensions._data[i]);
  }

  Context ctx;
  if (vkCreateInstance(&createInfo, nullptr, &ctx.instance) != VK_SUCCESS) {
    printf("failed to create instance\n");
  }

  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  VkExtensionProperties availableExtentions[extensionCount];
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtentions);

  for (const auto &extension : availableExtentions) {
    printf("\t%s\n", extension.extensionName);
  }

  if (debug) {
    setupDebugMessenger(ctx);
  }

  if (!SDL_Vulkan_CreateSurface(window, ctx.instance, nullptr, &ctx.surface)) {
    printf("failed to create window surface, %s", SDL_GetError());
    exit(0);
  }

  ctx.physical_device = pickSuitableDevice(scratch,ctx.instance, ctx.surface);
  vkGetPhysicalDeviceProperties(ctx.physical_device, &ctx.physical_device_properties);

  VkSampleCountFlags counts = ctx.physical_device_properties.limits.framebufferColorSampleCounts &
                              ctx.physical_device_properties.limits.framebufferDepthSampleCounts;

  ctx.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    ctx.msaaSamples = VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    ctx.msaaSamples = VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    ctx.msaaSamples = VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    ctx.msaaSamples = VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    ctx.msaaSamples = VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    ctx.msaaSamples = VK_SAMPLE_COUNT_2_BIT;
  }

  return ctx;
}
