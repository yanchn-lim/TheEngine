# Descriptors, Camera, and Materials

[[Vulkan Implementation Tutorial|Back to tutorial index]]

## Goal

Descriptors connect shader-visible resources to pipelines. Begin with one camera uniform buffer per frame, one material texture descriptor set, and model transforms as push constants.

```text
set 0, binding 0: per-frame camera uniform buffer
set 1, binding 0: material combined image sampler
push constants: per-draw model matrix
```

This keeps per-frame data safe for two frames in flight and avoids allocating a descriptor set for every submitted object.

## Camera Uniform Buffer

Use one host-visible uniform buffer and descriptor set for each `VulkanFrameResources`. Never overwrite a global camera buffer while an earlier frame can still read it.

```cpp
struct CameraUniform
{
    glm::mat4 viewProjection;
};

void VulkanRenderer::UpdateCameraBuffer(VulkanFrameResources& frame)
{
    CameraUniform data{};
    data.viewProjection = _camera.GetViewProjection();
    frame.cameraBuffer.Write(_device, &data, sizeof(data));
}
```

The matching vertex shader uses:

```glsl
layout(set = 0, binding = 0) uniform CameraData
{
    mat4 viewProjection;
} camera;

layout(push_constant) uniform DrawData
{
    mat4 model;
} draw;
```

Vulkan clip-space depth ranges from zero to one. Keep OpenGL camera math unchanged and apply Vulkan's projection convention in the Vulkan camera upload or shader policy. Decide one Y-axis policy early and use it consistently; do not globally alter GLM configuration and break OpenGL.

## Descriptor Layouts

```cpp
VkDescriptorSetLayoutBinding cameraBinding{};
cameraBinding.binding = 0;
cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
cameraBinding.descriptorCount = 1;
cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

VkDescriptorSetLayoutCreateInfo layoutInfo{};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = 1;
layoutInfo.pBindings = &cameraBinding;
VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_cameraSetLayout));
```

Create a descriptor pool with enough uniform-buffer sets for every frame and enough combined-image-sampler sets for all live materials. For an early engine implementation, recreate a material pool only on controlled asset reload or use a growable pool manager. Do not allocate descriptor sets every draw.

## Update a Camera Descriptor

```cpp
VkDescriptorBufferInfo bufferInfo{};
bufferInfo.buffer = frame.cameraBuffer.GetHandle();
bufferInfo.offset = 0;
bufferInfo.range = sizeof(CameraUniform);

VkWriteDescriptorSet write{};
write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
write.dstSet = frame.cameraDescriptorSet;
write.dstBinding = 0;
write.descriptorCount = 1;
write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
write.pBufferInfo = &bufferInfo;

vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
```

Update this once after creating each frame descriptor set. Each frame's buffer handle remains stable, so per-frame camera changes only map/copy data rather than rewriting descriptors.

## Pipeline Layout and Draw Binding

The pipeline layout owns the descriptor-set layout order and push-constant range. Its layout must match the shader's set/binding declarations exactly.

```cpp
VkPushConstantRange transformRange{};
transformRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
transformRange.offset = 0;
transformRange.size = sizeof(glm::mat4);

const VkDescriptorSetLayout setLayouts[] = {
    _cameraSetLayout,
    _materialSetLayout
};

VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 2;
pipelineLayoutInfo.pSetLayouts = setLayouts;
pipelineLayoutInfo.pushConstantRangeCount = 1;
pipelineLayoutInfo.pPushConstantRanges = &transformRange;
```

```cpp
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipelineLayout, 0, 1, &frame.cameraDescriptorSet, 0, nullptr);
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipelineLayout, 1, 1, &material.descriptorSet, 0, nullptr);
vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
    0, sizeof(glm::mat4), &drawCmd.transform);
```

`Assets::MaterialAsset` remains a backend-neutral description of shader, textures, and named values. `VulkanMaterial` is the resolved GPU form: pipeline plus descriptor set(s). Do not put Vulkan descriptor handles in the asset registry API.

## Verify

Draw the same mesh twice with separate transforms and one camera. Then bind two materials with different textures once texture upload is implemented. If descriptor validation fails, compare shader declarations, descriptor layout bindings, descriptor writes, and pipeline layout in that order.

Next: [[09 - Textures Depth and ImGui]].
