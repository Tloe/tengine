#include "context.h"

#include "arena.h"
#include "debug.h"
#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "ds_string.h"
#include "vulkan.h"
#include "vulkan/swap_chain.h"
#include "vulkan/vulkan.h"

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace {
  bool check_device_extension_support(mem::Arena* scratch, VkPhysicalDevice device) {
    auto available_extensions = vulkan::available_extensions(scratch, device);

    for (U32 i = 0; i < sizeof(vulkan::device_extensions) / sizeof(vulkan::device_extensions[0]);
         i++) {
      if (ds::array::contains(available_extensions,
                              ds::string::init(scratch, vulkan::device_extensions[i]))) {
        return false;
      }
    }
    return true;
  }

  bool is_device_suitable(mem::Arena* scratch, VkPhysicalDevice device, VkSurfaceKHR surface) {
    // TODO : Base device suitability checks at:
    // https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families

    U32 graphics_family, presentFamily;
    if (!vulkan::queue_family_indices(device, surface, &graphics_family, &presentFamily)) {
      return false;
    }

    if (check_device_extension_support(scratch, device)) {
      return false;
    }

    auto swap_chain_support = vulkan::swap_chain::swap_chain_support(scratch, device, surface);
    if (swap_chain_support.formats._size == 0 || swap_chain_support.present_modes._size == 0) {
      return false;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return supportedFeatures.samplerAnisotropy;
  }

  VkPhysicalDevice
  select_physical_device(mem::Arena* scratch, VkInstance instance, VkSurfaceKHR surface) {
    U32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0) {
      printf("failed to find GPUs with Vulkan support!");
      exit(0);
    }

    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    for (int i = 0; i < device_count; i++) {
      if (is_device_suitable(scratch, devices[i], surface)) {
        return devices[i];
      }
    }

    printf("failed to find a suitable GPU");
    exit(0);

    return VK_NULL_HANDLE;
  }

  void create_logical_device(mem::Arena* scratch, bool debug, vulkan::Context& ctx) {
    U32 graphics_family_index, present_family_index;
    vulkan::queue_family_indices(ctx.physical_device,
                                 ctx.surface,
                                 &graphics_family_index,
                                 &present_family_index);

    auto unique_queue_families = ds::array::init<U32>(scratch);
    ds::array::push_back_unique(unique_queue_families, graphics_family_index);
    ds::array::push_back_unique(unique_queue_families, present_family_index);

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

    VkDeviceCreateInfo create_info{};
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount    = unique_queue_families._size;
    create_info.pQueueCreateInfos       = queue_create_infos;
    create_info.pEnabledFeatures        = &device_features;
    create_info.enabledExtensionCount   = ds::array::size(vulkan::device_extensions);
    create_info.ppEnabledExtensionNames = &vulkan::device_extensions[0];

    if (debug) {
      create_info.enabledLayerCount   = ds::array::size(vulkan::validation_layers);
      create_info.ppEnabledLayerNames = &vulkan::validation_layers[0];
    } else {
      create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(ctx.physical_device, &create_info, nullptr, &ctx.logical_device) !=
        VK_SUCCESS) {
      printf("failed to create logical device!");
      exit(0);
    }

    vkGetDeviceQueue(ctx.logical_device, graphics_family_index, 0, &ctx.graphics_queue);
    vkGetDeviceQueue(ctx.logical_device, present_family_index, 0, &ctx.present_queue);
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
} // namespace

vulkan::Context vulkan::context::create(mem::Arena* scratch, SDL_Window* window, bool debug) {
  VkApplicationInfo app_info{};
  app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName   = "tengine";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName        = "tengine";
  app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion         = VK_API_VERSION_1_0;

  VkInstanceCreateInfo instance_create_info{};
  instance_create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo = &app_info;

  auto required_extensions                     = vulkan::required_extension(scratch, debug);
  instance_create_info.enabledExtensionCount   = required_extensions._size;
  instance_create_info.ppEnabledExtensionNames = required_extensions._data;

  if (debug) {
    printf("DEBUG\n");
    instance_create_info.enabledLayerCount =
        static_cast<uint32_t>(sizeof(validation_layers) / sizeof(validation_layers[0]));
    instance_create_info.ppEnabledLayerNames = validation_layers;

    auto debug_create_info     = debug::debug_messenger_create_info();
    instance_create_info.pNext = &debug_create_info;
  } else {
    instance_create_info.enabledLayerCount = 0;
    instance_create_info.pNext             = nullptr;
  }

  printf("required extensions:\n");
  for (int i = 0; i < required_extensions._size; i++) {
    printf("\t%s\n", required_extensions._data[i]);
  }

  Context ctx;
  if (vkCreateInstance(&instance_create_info, nullptr, &ctx.instance) != VK_SUCCESS) {
    printf("failed to create instance\n");
    exit(0);
  }

  U32 extension_count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

  VkExtensionProperties available_extentions[extension_count];
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extentions);

  for (U32 i = 0; i < extension_count; i++) {
    printf("\t%s\n", available_extentions[i].extensionName);
  }

  if (debug) {
    vulkan::debug::init(ctx.instance);
  }

  if (!SDL_Vulkan_CreateSurface(window, ctx.instance, nullptr, &ctx.surface)) {
    printf("failed to create window surface, %s", SDL_GetError());
    exit(0);
  }

  ctx.physical_device = select_physical_device(scratch, ctx.instance, ctx.surface);

  ctx.max_msaa_samples = max_msaa_samples(ctx.physical_device);

  create_logical_device(scratch, debug, ctx);

  U32 graphics_family, present_family;
  vulkan::queue_family_indices(ctx.physical_device, ctx.surface, &graphics_family, &present_family);

  VkCommandPoolCreateInfo pool_create_info{};
  pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_create_info.queueFamilyIndex = graphics_family;

  if (vkCreateCommandPool(ctx.logical_device, &pool_create_info, nullptr, &ctx.command_pool) !=
      VK_SUCCESS) {
    printf("failed to create command pool!");
    exit(0);
  }

  return ctx;
}

void vulkan::context::cleanup(Context& ctx, bool debug) {
  vkDestroyDevice(ctx.logical_device, nullptr);

  if (debug) {
    vulkan::debug::cleanup(ctx.instance);
  }

  vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
  vkDestroyInstance(ctx.instance, nullptr);
}

VkSurfaceFormatKHR vulkan::context::surface_format(vulkan::ContextPtr ctx) {
  U32 format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device, ctx->surface, &format_count, nullptr);

  assert(format_count != 0);

  VkSurfaceFormatKHR available_formats[format_count];
  vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device,
                                       ctx->surface,
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
