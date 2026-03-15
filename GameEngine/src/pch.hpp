#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

// Standard library
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <map>
#include <memory>
#include <functional>
#include <algorithm>
#include <utility>
#include <cassert>
#include <cstdint>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// GLM type aliases
using float2 = glm::f32vec2;
using float3 = glm::f32vec3;
using float4 = glm::f32vec4;
using int2 = glm::i32vec2;
using int3 = glm::i32vec3;
using int4 = glm::i32vec4;
using mat3 = glm::mat3;
using mat4 = glm::mat4;
using quat = glm::quat;