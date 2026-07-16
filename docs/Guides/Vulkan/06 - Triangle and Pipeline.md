# Triangle and Pipeline

[[Vulkan Implementation Tutorial|Back to tutorial index]]

## Goal

A Vulkan graphics pipeline combines shader stages and fixed state that OpenGL often keeps as mutable global state. Create one hardcoded triangle first. It proves shader compilation, SPIR-V loading, dynamic-rendering pipeline creation, viewport/scissor setup, and draw recording before mesh uploads or `DrawCmd` resolution enter the picture.

## Compile Shaders Offline

Vulkan does not compile GLSL source at runtime. Compile shader source to SPIR-V during the asset/build step:

```text
glslc triangle.vert -o triangle.vert.spv
glslc triangle.frag -o triangle.frag.spv
```

The first vertex shader can generate positions from `gl_VertexIndex`, so no vertex buffer is needed yet:

```glsl
#version 450

layout(location = 0) out vec3 outColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    outColor = vec3(1.0, 0.7, 0.2);
}
```

## Shader Modules

Read the file as binary bytes. SPIR-V data must be 4-byte aligned and its size must be a multiple of four.

```cpp
bool VulkanShaderModule::Create(VkDevice device, const std::vector<uint32_t>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    return vkCreateShaderModule(device, &createInfo, nullptr, &_module) == VK_SUCCESS;
}
```

Treat `VkShaderModule` as an input to pipeline creation. Destroy it after the pipeline is successfully created unless you keep it for pipeline rebuilds or hot reload.

## Pipeline Layout and Dynamic Rendering

The first pipeline has no descriptors or push constants, so its layout is empty. The pipeline's rendering information must name the swapchain color format.

```cpp
VkPipelineLayoutCreateInfo layoutInfo{};
layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &_layout));

VkPipelineRenderingCreateInfo renderingInfo{};
renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
renderingInfo.colorAttachmentCount = 1;
renderingInfo.pColorAttachmentFormats = &swapchainFormat;

VkGraphicsPipelineCreateInfo pipelineInfo{};
pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
pipelineInfo.pNext = &renderingInfo;
pipelineInfo.layout = _layout;
pipelineInfo.renderPass = VK_NULL_HANDLE;
```

Build the remaining pipeline-state structures deliberately: vertex input is empty, input assembly is `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`, rasterization fills polygons, multisampling is one sample, color writes enable RGBA, and depth testing is disabled for now.

Make viewport and scissor dynamic so a resize does not recreate the pipeline solely for a new extent:

```cpp
const VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
};

VkPipelineDynamicStateCreateInfo dynamicInfo{};
dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
dynamicInfo.dynamicStateCount = 2;
dynamicInfo.pDynamicStates = dynamicStates;
```

## Record the Triangle

Record the draw between `vkCmdBeginRendering` and `vkCmdEndRendering`:

```cpp
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);

VkViewport viewport{ 0.0f, 0.0f,
    static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f };
VkRect2D scissor{ { 0, 0 }, extent };
vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

vkCmdDraw(commandBuffer, 3, 1, 0, 0);
```

## Verify

A visible triangle proves the core frame path. If it is missing, check cull mode and vertex winding before changing synchronization. A typical first pipeline uses `VK_CULL_MODE_NONE` to remove winding as a variable.

Next: [[07 - Buffers and Meshes]].
