# Frames and Synchronization

[[Vulkan Implementation Tutorial|Back to tutorial index]]

## Goal

Vulkan CPU calls return before the GPU finishes. Frame resources prevent the CPU from resetting a command pool or semaphore while the GPU still uses it. Begin with two frames in flight.

```cpp
struct VulkanFrameResources
{
    VkCommandPool commandPool{ VK_NULL_HANDLE };
    VkCommandBuffer commandBuffer{ VK_NULL_HANDLE };
    VkFence inFlightFence{ VK_NULL_HANDLE };
    VkSemaphore imageAvailable{ VK_NULL_HANDLE };
    VkSemaphore renderFinished{ VK_NULL_HANDLE };
};

constexpr uint32_t FramesInFlight = 2;
std::array<VulkanFrameResources, FramesInFlight> _frames;
uint32_t _frameIndex = 0;
uint32_t _imageIndex = 0;
```

The command pool belongs to a frame because resetting a command pool resets all command buffers allocated from it. The fence protects reuse of that frame. Semaphores connect swapchain acquisition, GPU work, and presentation.

## Create Frame Resources

Create each command pool for the graphics queue family. Allocate one primary command buffer from it. Create a fence in the signaled state so the first frame does not block.

```cpp
VkCommandPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
poolInfo.queueFamilyIndex = device.GetGraphicsQueueFamily();
poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

VkFenceCreateInfo fenceInfo{};
fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

VkSemaphoreCreateInfo semaphoreInfo{};
semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
```

Destroy fence and semaphores before the command pool. Destroy every frame resource before the logical device.

## BeginFrame

`IRenderer::BeginFrame()` is currently `void`, so retain that interface and store whether recording can proceed in renderer state. It waits for its frame fence, acquires a swapchain image, resets resources, and begins command recording. It must not reset the fence until acquisition has succeeded; otherwise an out-of-date return can leave the current frame permanently unsignaled.

```cpp
void VulkanRenderer::BeginFrame()
{
    _frameReady = false;
    VulkanFrameResources& frame = _frames[_frameIndex];
    VK_CHECK(vkWaitForFences(_device.GetHandle(), 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX));

    const VkResult acquireResult = vkAcquireNextImageKHR(
        _device.GetHandle(), _swapchain.GetHandle(), UINT64_MAX,
        frame.imageAvailable, VK_NULL_HANDLE, &_imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        _resizePending = true;
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        return;
    }

    VK_CHECK(vkResetFences(_device.GetHandle(), 1, &frame.inFlightFence));
    VK_CHECK(vkResetCommandPool(_device.GetHandle(), frame.commandPool, 0));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(frame.commandBuffer, &beginInfo));
    _frameReady = true;
}
```

Guard `Submit`, `EndFrame`, and `Present` with `_frameReady`. If acquire returns `VK_SUBOPTIMAL_KHR`, draw this frame and schedule recreation. If it returns `VK_ERROR_OUT_OF_DATE_KHR`, skip recording and recreate at the safe boundary.

## Synchronization 2 Submission

Finish command recording in `EndFrame`, then submit it with `vkQueueSubmit2`. The acquire semaphore waits before color-attachment writes. The render-finished semaphore signals after color output completes and is waited on by presentation.

```cpp
VkCommandBufferSubmitInfo commandInfo{};
commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
commandInfo.commandBuffer = frame.commandBuffer;

VkSemaphoreSubmitInfo waitInfo{};
waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
waitInfo.semaphore = frame.imageAvailable;
waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

VkSemaphoreSubmitInfo signalInfo{};
signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
signalInfo.semaphore = frame.renderFinished;
signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

VkSubmitInfo2 submitInfo{};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
submitInfo.waitSemaphoreInfoCount = 1;
submitInfo.pWaitSemaphoreInfos = &waitInfo;
submitInfo.commandBufferInfoCount = 1;
submitInfo.pCommandBufferInfos = &commandInfo;
submitInfo.signalSemaphoreInfoCount = 1;
submitInfo.pSignalSemaphoreInfos = &signalInfo;

VK_CHECK(vkQueueSubmit2(_device.GetGraphicsQueue(), 1, &submitInfo, frame.inFlightFence));
```

Use `VkImageMemoryBarrier2` with `vkCmdPipelineBarrier2`, not old `VkImageMemoryBarrier` or `vkQueueSubmit` structures. Mixing the models makes later synchronization harder to reason about.

## Present

```cpp
void VulkanRenderer::Present()
{
    VulkanFrameResources& frame = _frames[_frameIndex];
    const VkSwapchainKHR swapchain = _swapchain.GetHandle();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &frame.renderFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &_imageIndex;

    const VkResult result = vkQueuePresentKHR(_device.GetPresentQueue(), &presentInfo);
    _resizePending |= result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR;
    _frameIndex = (_frameIndex + 1) % FramesInFlight;
}
```

Do not advance `_frameIndex` when `_frameReady` is false because no frame was submitted.

## Verify

With no rendering commands yet, command buffers should begin, end, submit, and present with validation clean. A black or undefined image is expected until the next chapter records a clear pass.

Next: [[05 - Clear Frame and Dynamic Rendering]].
