#include "context.h"

#include "arena.h"
#include "common.h"
#include "debug.h"
#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "ds_string.h"
#include "handles.h"
#include "swap_chain.h"
#include "vulkan_include.h"

#include <SDL3/SDL_vulkan.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace vulkan { Context _ctx = {}; }

namespace {
  ArenaHandle mem_render           = arena::by_name("render");

  bool check_device_extension_support(VkPhysicalDevice device) {
    auto available = vulkan::available_extensions(device);

    for (U32 i = 0; i < array::size(vulkan::device_extensions); i++) {
      if (array::contains(available,
                          string::init(arena::scratch(), vulkan::device_extensions[i]))) {
        return false;
      }
    }
    return true;
  }

  bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    // TODO : Base device suitability checks at:
    // https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families

    U32 graphics_family, presentFamily;
    if (!vulkan::queue_family_indices(device, surface, &graphics_family, &presentFamily)) {
      return false;
    }

    if (check_device_extension_support(device)) {
      return false;
    }

    auto swap_chain_support = vulkan::swap_chain::swap_chain_support(device, surface);
    if (swap_chain_support.formats._size == 0 || swap_chain_support.present_modes._size == 0) {
      return false;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return supportedFeatures.samplerAnisotropy;
  }

  VkPhysicalDevice select_physical_device(VkInstance instance, VkSurfaceKHR surface) {
    U32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0) {
      printf("failed to find GPUs with Vulkan support!");
      exit(0);
    }

    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    for (int i = 0; i < device_count; i++) {
      if (is_device_suitable(devices[i], surface)) {
        return devices[i];
      }
    }

    printf("failed to find a suitable GPU");
    exit(0);

    return VK_NULL_HANDLE;
  }

  void create_logical_device(bool debug) {
    U32 graphics_family_index, present_family_index;
    vulkan::queue_family_indices(vulkan::_ctx.physical_device,
                                 vulkan::_ctx.surface,
                                 &graphics_family_index,
                                 &present_family_index);

    auto unique_queue_families = S_DARRAY_EMPTY(U32);

    array::push_back_unique(unique_queue_families, graphics_family_index);
    array::push_back_unique(unique_queue_families, present_family_index);

    F32                     queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_infos[unique_queue_families._size];

    for (U32 i = 0; i < unique_queue_families._size; i++) {
      queue_create_infos[i]                  = VkDeviceQueueCreateInfo{};
      queue_create_infos[i].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_infos[i].queueFamilyIndex = unique_queue_families._data[i];
      queue_create_infos[i].queueCount       = 1;
      queue_create_infos[i].pQueuePriorities = &queue_priority;
    }

    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;
    device_features.sampleRateShading = VK_TRUE;

    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{};
    descriptor_indexing_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.descriptorBindingPartiallyBound           = VK_TRUE;
    descriptor_indexing_features.runtimeDescriptorArray                    = VK_TRUE;

    VkPhysicalDeviceFeatures2 device_features2{};
    device_features2.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device_features2.features = device_features;
    device_features2.pNext    = &descriptor_indexing_features;

    VkDeviceCreateInfo create_info{};
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount    = unique_queue_families._size;
    create_info.pQueueCreateInfos       = queue_create_infos;
    create_info.pEnabledFeatures        = nullptr;
    create_info.enabledExtensionCount   = array::size(vulkan::device_extensions);
    create_info.ppEnabledExtensionNames = &vulkan::device_extensions[0];
    create_info.pNext                   = &device_features2;

    if (debug) {
      create_info.enabledLayerCount   = array::size(vulkan::validation_layers);
      create_info.ppEnabledLayerNames = &vulkan::validation_layers[0];
    } else {
      create_info.enabledLayerCount = 0;
    }

    ASSERT_SUCCESS("failed to create logical device!",
                   vkCreateDevice(vulkan::_ctx.physical_device,
                                  &create_info,
                                  nullptr,
                                  &vulkan::_ctx.logical_device));

    volkLoadDevice(vulkan::_ctx.logical_device);

    vkGetDeviceQueue(vulkan::_ctx.logical_device,
                     graphics_family_index,
                     0,
                     &vulkan::_ctx.graphics_queue);
    vkGetDeviceQueue(vulkan::_ctx.logical_device,
                     present_family_index,
                     0,
                     &vulkan::_ctx.present_queue);
  }

  VkSampleCountFlagBits max_msaa_samples(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_device, &props);
    VkSampleCountFlags counts =
        props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) {
      return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT) {
      return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT) {
      return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT) {
      return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT) {
      return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT) {
      return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
  }
}

void vulkan::context::init(SDL_Window* window, bool debug) {
  volkInitialize();
  VkApplicationInfo app_info{};
  app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName   = "tengine";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName        = "tengine";
  app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion         = VK_API_VERSION_1_2;

  // Instance create info
  VkInstanceCreateInfo instance_create_info{};
  instance_create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo = &app_info;

  // Extensions
  auto instance_extensions                     = required_instance_extension(debug);
  instance_create_info.enabledExtensionCount   = instance_extensions._size;
  instance_create_info.ppEnabledExtensionNames = instance_extensions._data;

  // Layers: only if debug and if available
  VkDebugUtilsMessengerCreateInfoEXT debug_ci{};
  if (debug) {
    // 1. Query how many layers there are
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // 2. Allocate a vector and fetch their properties
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    // 3. Check for “VK_LAYER_KHRONOS_validation”
    bool haveValidation = false;
    for (const auto& lp : layers) {
      if (std::strcmp(lp.layerName, validation_layers[0]) == 0) {
        haveValidation = true;
        break;
      }
    }

    if (haveValidation) {
      std::cout << "DEBUG: enabling validation layer\n";
      instance_create_info.enabledLayerCount   = 1;
      instance_create_info.ppEnabledLayerNames = validation_layers;
    } else {
      std::cout << "DEBUG: validation layer not found, skipping\n";
      instance_create_info.enabledLayerCount   = 0;
      instance_create_info.ppEnabledLayerNames = nullptr;
    }

    // Chain in the debug messenger
    debug_ci              = debug::debug_messenger_create_info();
    instance_create_info.pNext = &debug_ci;
  } else {
    instance_create_info.enabledLayerCount   = 0;
    instance_create_info.ppEnabledLayerNames = nullptr;
    instance_create_info.pNext               = nullptr;
  }

  // Initialize Volk and create instance
  ASSERT_SUCCESS("failed to create instance",
                 vkCreateInstance(&instance_create_info, nullptr, &_ctx.instance));
  volkLoadInstance(_ctx.instance);

  U32 extension_count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

  VkExtensionProperties available_extentions[extension_count];
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extentions);

  for (U32 i = 0; i < extension_count; i++) {
    printf("\t%s\n", available_extentions[i].extensionName);
  }

  if (debug) {
    vulkan::debug::init(_ctx.instance);
  }

  if (!SDL_Vulkan_CreateSurface(window, _ctx.instance, nullptr, &_ctx.surface)) {
    printf("failed to create window surface, %s", SDL_GetError());
    exit(0);
  }

  _ctx.physical_device = select_physical_device(_ctx.instance, _ctx.surface);

  _ctx.max_msaa_samples = max_msaa_samples(_ctx.physical_device);

  create_logical_device(debug);
}

void vulkan::context::cleanup(bool debug) {
  vkDestroyDevice(vulkan::_ctx.logical_device, nullptr);

  if (debug) {
    vulkan::debug::cleanup(vulkan::_ctx.instance);
  }

  SDL_Vulkan_DestroySurface(vulkan::_ctx.instance, vulkan::_ctx.surface, nullptr);

  vkDestroyInstance(vulkan::_ctx.instance, nullptr);
}

VkSurfaceFormatKHR vulkan::context::surface_format() {
  U32 format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan::_ctx.physical_device,
                                       vulkan::_ctx.surface,
                                       &format_count,
                                       nullptr);

  assert(format_count != 0);

  VkSurfaceFormatKHR available_formats[format_count];
  vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan::_ctx.physical_device,
                                       vulkan::_ctx.surface,
                                       &format_count,
                                       available_formats);

  for (U32 i = 0; i < format_count; i++) {
    if (available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return available_formats[i];
    }
  }

  return available_formats[0];
}
