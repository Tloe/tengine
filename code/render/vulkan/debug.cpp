#include "debug.h"
#include "ds_array_static.h"
#include "vulkan.h"
#include <cstdio>
#include <cstdlib>
#include <types.h>
#include <vulkan/vulkan_core.h>

namespace {
  VkDebugUtilsMessengerEXT debugMessenger;

  bool checkValidationLayerSupport() {
    U32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    VkLayerProperties available_layers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, available_layers);

    for (U32 i = 0; i < ds::array::size(vulkan::validation_layers); i++) {
      bool layerFound = false;

      for (U32 j = 0; j < layerCount; j++) {
        if (strcmp(vulkan::validation_layers[i], available_layers[j].layerName) == 0) {
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
    fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
  }
}

void vulkan::debug::init(VkInstance instance) {
  if (!checkValidationLayerSupport()) {
    printf("validation layers requested, but not available!");
    exit(0);
  }
  auto debug_create_info = debug::debug_messenger_create_info();

  auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

  VkResult result;
  if (fn != nullptr) {
    result = fn(instance, &debug_create_info, nullptr, &debugMessenger);
  } else {
    result = VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  if (result != VK_SUCCESS) {
    printf("failed to setup debug messenger!");
    exit(0);
  }
}

void vulkan::debug::cleanup(VkInstance instance) {
  auto func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                 "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, nullptr);
  }
}

VkDebugUtilsMessengerCreateInfoEXT vulkan::debug::debug_messenger_create_info() {
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
