#pragma once

// centralize Vulkan-Hpp configuration so every Vulkan translation unit agrees
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS

#include <vulkan/vulkan_raii.hpp>
