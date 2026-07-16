# Clear Frame and Dynamic Rendering

[[Vulkan Implementation Tutorial|Back to tutorial index]]

## Goal

Before a triangle, prove that the entire frame path works: acquire an image, transition its layout, clear it through dynamic rendering, submit it, and present it. This removes pipeline and mesh complexity from the first visual Vulkan test.

Dynamic rendering replaces the main-path render-pass/framebuffer setup. Do not create a `VkRenderPass` or `VkFramebuffer` for this renderer path.

## Image Layouts

A swapchain image starts each rendered frame in `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`. Color attachment rendering requires `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`. Vulkan does not perform this transition implicitly, so record two barriers every frame.

```cpp
void TransitionSwapchainImage(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags2 srcStage,
    VkAccessFlags2 srcAccess,
    VkPipelineStageFlags2 dstStage,
    VkAccessFlags2 dstAccess)
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = srcStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}
```

Before rendering, use `VK_PIPELINE_STAGE_2_NONE` and access `0` as the source, then `VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT` and `VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT` as the destination. Before present, reverse the dependency: source is color attachment output/write and destination is `VK_PIPELINE_STAGE_2_NONE` with access `0`.

## Record a Clear Pass

```cpp
void VulkanRenderer::RecordClear(VkCommandBuffer commandBuffer)
{
    const VkExtent2D extent = _swapchain.GetExtent();

    TransitionSwapchainImage(
        commandBuffer, _swapchain.GetImage(_imageIndex),
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_NONE, 0,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = _swapchain.GetImageView(_imageIndex);
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = { { 0.05f, 0.08f, 0.12f, 1.0f } };

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = { { 0, 0 }, extent };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    vkCmdEndRendering(commandBuffer);

    TransitionSwapchainImage(
        commandBuffer, _swapchain.GetImage(_imageIndex),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_NONE, 0);
}
```

Call this after beginning the command buffer and before `vkEndCommandBuffer`. The clear color should appear once `EndFrame` records the pass and `Present` submits it.

## Common Errors

If validation says an image layout is wrong, inspect every earlier use of that image and the exact barrier layouts. If the window stays black but validation is clean, verify that `Present()` waits for `renderFinished`, and verify the clear pass is recorded before command-buffer end.

Do not use `VK_IMAGE_LAYOUT_UNDEFINED` as the old layout for a swapchain image after the first present. That discards previous contents and conflicts with the present engine's tracked layout.

## Verify

The window should show a stable clear color over hundreds of frames. Resize and minimize/restore should recreate the swapchain and continue clearing without validation errors.

Next: [[06 - Triangle and Pipeline]].
