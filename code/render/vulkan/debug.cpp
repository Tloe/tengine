#include "debug.h"

#include "ds_array_static.h"
#include "vulkan.h"

#include <cstdio>
#include <cstdlib>
#include <types.h>
#include <vulkan/vulkan_core.h>

namespace {
  VkDebugUtilsMessengerEXT debug_messenger;

  bool check_validation_layer_support() {
    U32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    VkLayerProperties available_layers[layer_count];
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for (U32 i = 0; i < array::size(vulkan::validation_layers); i++) {
      bool layer_found = false;

      for (U32 j = 0; j < layer_count; j++) {
        if (strcmp(vulkan::validation_layers[i], available_layers[j].layerName) == 0) {
          layer_found = true;
          break;
        }
      }

      if (!layer_found) {
        return false;
      }
    }

    return true;
  }

  VKAPI_ATTR VkBool32 VKAPI_CALL
  debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                 VkDebugUtilsMessageTypeFlagsEXT             message_type,
                 const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                 void*                                       user_data) {
    fprintf(stderr, "validation layer: %s\n", callback_data->pMessage);

    return VK_FALSE;
  }
}

void vulkan::debug::init(VkInstance instance) {
  if (!check_validation_layer_support()) {
    printf("validation layers requested, but not available!");
    exit(0);
  }
  auto debug_create_info = debug::debug_messenger_create_info();

  auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

  VkResult result;
  if (fn != nullptr) {
    result = fn(instance, &debug_create_info, nullptr, &debug_messenger);
  } else {
    result = VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  ASSERT_SUCCESS("failed to setup debug messenger!", result);
}

void vulkan::debug::cleanup(VkInstance instance) {
  auto func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                 "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debug_messenger, nullptr);
  }
}

VkDebugUtilsMessengerCreateInfoEXT vulkan::debug::debug_messenger_create_info() {
  VkDebugUtilsMessengerCreateInfoEXT create_info{};

  create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debug_callback;

  return create_info;
}
