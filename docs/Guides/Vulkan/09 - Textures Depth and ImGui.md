# Textures, Depth, and ImGui

[[Vulkan Implementation Tutorial|Back to tutorial index]]

## Texture Upload

`VulkanImage` owns `VkImage`, its memory, and an image view. `VulkanTexture2D` owns a `VulkanImage` plus a sampler. Existing `Texture2D` source pixels and `TextureHandle` remain backend-neutral.

Upload pixels through a staging buffer, as with mesh data:

```text
stb_image pixels
    -> host-visible staging VkBuffer
    -> image: undefined to transfer destination
    -> vkCmdCopyBufferToImage
    -> image: transfer destination to shader read only
    -> VkImageView + VkSampler + material descriptor
```

Create sampled images with these usage bits:

```cpp
imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                  VK_IMAGE_USAGE_SAMPLED_BIT;
```

The first transition uses `VK_IMAGE_LAYOUT_UNDEFINED` because no prior contents matter for a newly created image. The final transition uses `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`, with a destination stage of fragment shader and destination access of shader sampled read.

```cpp
VkBufferImageCopy region{};
region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
region.imageSubresource.mipLevel = 0;
region.imageSubresource.layerCount = 1;
region.imageExtent = { width, height, 1 };
vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
```

Write the texture into the material descriptor set after the image view and sampler exist:

```cpp
VkDescriptorImageInfo imageInfo{};
imageInfo.sampler = texture.GetSampler();
imageInfo.imageView = texture.GetImageView();
imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

VkWriteDescriptorSet write{};
write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
write.dstSet = materialSet;
write.dstBinding = 0;
write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
write.descriptorCount = 1;
write.pImageInfo = &imageInfo;
vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
```

## Depth

Create a depth image whenever the swapchain extent changes. The depth format must support `VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT`; start by testing `VK_FORMAT_D32_SFLOAT`, then fall back through supported candidates.

The depth image needs `VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT`, a depth-aspect image view, and a transition to `VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL`. Add it to dynamic rendering:

```cpp
VkRenderingAttachmentInfo depthAttachment{};
depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
depthAttachment.imageView = _depthImage.GetView();
depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

renderingInfo.pDepthAttachment = &depthAttachment;
```

Pipeline depth state must be enabled as well:

```cpp
VkPipelineDepthStencilStateCreateInfo depthState{};
depthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
depthState.depthTestEnable = VK_TRUE;
depthState.depthWriteEnable = VK_TRUE;
depthState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
```

Destroy depth resources before recreating the swapchain-dependent rendering targets. Recreate pipelines if their rendering attachment formats change.

## Integrate Existing Meshes and Materials

Only now adapt `MeshUploadData`, OBJ imports, textures, and material fallback resources. Resolve backend-neutral render data inside `VulkanRenderer::Submit` or command recording:

```cpp
void VulkanRenderer::RecordDraw(const Graphics::DrawCmd& drawCmd)
{
    const VulkanMesh* mesh = _resolver.TryResolveMesh(drawCmd.mesh);
    const VulkanMaterial* material = _resolver.TryResolveMaterial(drawCmd.material);
    if (mesh == nullptr || material == nullptr)
    {
        return;
    }

    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->GetPipeline());
    material->Bind(_commandBuffer, _currentFrame);
    mesh->Bind(_commandBuffer);
    vkCmdDrawIndexed(_commandBuffer, mesh->GetIndexCount(), 1, 0, 0, 0);
}
```

Keep the existing fallback material policy. A fallback Vulkan material must use a vertex layout compatible with the mesh it receives; it cannot magically interpret arbitrary vertex data.

## ImGui

Add ImGui last. The Vulkan path uses `imgui_impl_glfw` and `imgui_impl_vulkan`, a descriptor pool large enough for ImGui, and the active command buffer. Record `ImGui_ImplVulkan_RenderDrawData` after engine world draws and before `vkCmdEndRendering`, then make the final transition to present.

ImGui is a backend integration detail. Keep the profiler and console UI code backend-neutral, but keep Vulkan ImGui initialization, font upload, descriptor pool, and shutdown in the Vulkan backend.

## Final Verification

The Vulkan track is complete when OpenGL and Vulkan are selectable, validation is clean, resize/minimize/shutdown work, dynamic rendering and Synchronization 2 are used, indexed engine meshes and textures render through `DrawCmd`, camera and transforms work, depth testing works, and ImGui overlays render before presentation.

Do not move Vulkan types into Scene, future ECS, importers, asset handles, or backend-neutral graphics headers just to make a call site convenient.
