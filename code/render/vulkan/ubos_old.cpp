#include "ubos.h"

#include "buffers.h"
#include "context.h"
#include "ds_hashmap.h"
#include "handles.h"

#include <vulkan/vulkan_core.h>

/* namespace { */
/*   auto                mem_render_resources = arena::by_name("render_resources"); */
/*   HashMap<U16, void*> cpu_memory_ptrs = hashmap::init<U16, void*>(mem_render_resources); */
/* } */
/*  */
/* vulkan::UBOBufferHandle vulkan::ubos::create(U32 byte_size) { */
/*   VkDeviceSize size = byte_size; */
/*  */
/*   auto buffer_handle = */
/*       buffers::create(size, */
/*                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, */
/*                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); */
/*  */
/*   auto cpu_memory_ptr = hashmap::insert(cpu_memory_ptrs, buffer_handle.value); */
/*  */
/*   vkMapMemory(vulkan::_ctx.logical_device, *buffers::memory(buffer_handle), 0, size, 0, cpu_memory_ptr); */
/*  */
/*   return UBOBufferHandle{.value = buffer_handle.value}; */
/* } */
/*  */
/* void vulkan::ubos::cleanup(UBOBufferHandle handle) { */
/*   buffers::cleanup(BufferHandle{.value = handle.value}); */
/* } */
/*  */
/* void* vulkan::ubos::cpu_memory(UBOBufferHandle handle) { */
/*   return *hashmap::value(cpu_memory_ptrs, handle.value); */
/* } */
