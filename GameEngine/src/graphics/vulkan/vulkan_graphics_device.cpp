#include "vulkan_graphics_device.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include <GLFW/glfw3.h>

#include "debug/debug.hpp"

namespace
{
    // matches the push-constant layout declared by the Vulkan shaders
    struct VulkanDrawConstants
    {
        glm::mat4 mvp{ 1.0f };
        glm::vec4 tint{ 1.0f };
        glm::vec4 uvRect{ 0.0f, 0.0f, 1.0f, 1.0f };
    };

    vk::PrimitiveTopology ToVulkanTopology(Graphics::PrimitiveTopology topology)
    {
        switch (topology)
        {
        case Graphics::PrimitiveTopology::LINES: return vk::PrimitiveTopology::eLineList;
        case Graphics::PrimitiveTopology::POINTS: return vk::PrimitiveTopology::ePointList;
        default: return vk::PrimitiveTopology::eTriangleList;
        }
    }

    vk::Format ToVulkanFormat(Graphics::ShaderDataType type)
    {
        switch (type)
        {
        case Graphics::ShaderDataType::FLOAT: return vk::Format::eR32Sfloat;
        case Graphics::ShaderDataType::FLOAT2: return vk::Format::eR32G32Sfloat;
        case Graphics::ShaderDataType::FLOAT3: return vk::Format::eR32G32B32Sfloat;
        case Graphics::ShaderDataType::FLOAT4: return vk::Format::eR32G32B32A32Sfloat;
        case Graphics::ShaderDataType::INT: return vk::Format::eR32Sint;
        case Graphics::ShaderDataType::INT2: return vk::Format::eR32G32Sint;
        case Graphics::ShaderDataType::INT3: return vk::Format::eR32G32B32Sint;
        case Graphics::ShaderDataType::INT4: return vk::Format::eR32G32B32A32Sint;
        default: return vk::Format::eUndefined;
        }
    }

    Graphics::FrameStatus FromVulkanError(const vk::SystemError& error)
    {
        return error.code().value() == VK_ERROR_DEVICE_LOST
            ? Graphics::FrameStatus::DeviceLost
            : Graphics::FrameStatus::Fatal;
    }
}

namespace Graphics
{
    size_t VulkanGraphicsDevice::TextureSetKeyHash::operator()(const TextureSetKey& key) const
    {
        // combine every handle field so descriptor caching cannot alias reused resource slots
        size_t hash = key.texture.index;
        const auto combine = [&hash](uint32_t value)
        {
            hash ^= static_cast<size_t>(value) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
        };
        combine(key.texture.generation);
        combine(key.texture.owner);
        combine(key.sampler.index);
        combine(key.sampler.generation);
        combine(key.sampler.owner);
        return hash;
    }

    bool VulkanGraphicsDevice::Initialize(const GraphicsDeviceDesc& desc)
    {
        // create Vulkan owners from the window outward before frame resources
        _window = static_cast<GLFWwindow*>(desc.window);
        if (!_window || !_context.Init(_window) || !_device.Init(_context)) return false;
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(_window, &width, &height);
        if (width <= 0 || height <= 0) return false;
        _requestedExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        if (!_swapchain.Create(_device, _context.SurfaceHandle(), _requestedExtent)) return false;
        try
        {
            _depthFormat = FindDepthFormat();
            CreateFrameResources();
            CreateDepthResources();
            CreateDescriptors();
            return true;
        }
        catch (const std::exception& error)
        {
            Debug::LogError("VulkanGraphicsDevice::Initialize: ", error.what());
            Shutdown();
            return false;
        }
    }

    GpuBufferHandle VulkanGraphicsDevice::CreateBuffer(const BufferDesc& desc)
    {
        if (!desc.data || desc.size == 0) return {};
        vk::BufferUsageFlags usage = desc.usage == BufferUsage::Index
            ? vk::BufferUsageFlagBits::eIndexBuffer : vk::BufferUsageFlagBits::eVertexBuffer;
        if (desc.usage == BufferUsage::Uniform) usage = vk::BufferUsageFlagBits::eUniformBuffer;
        try
        {
            return _buffers.Create(CreateBufferResource(desc.data, desc.size, usage,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
        }
        catch (const std::exception& error)
        {
            Debug::LogError("Vulkan buffer creation failed: ", error.what());
            return {};
        }
    }

    GpuTextureHandle VulkanGraphicsDevice::CreateTexture(const TextureDesc& desc)
    {
        // stage CPU pixels, transition the image, copy data, and expose one sampled view
        if (!desc.pixels || !desc.width || !desc.height) return {};
        try
        {
            const size_t byteCount = static_cast<size_t>(desc.width) * desc.height * 4;
            BufferResource staging = CreateBufferResource(desc.pixels, byteCount,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

            vk::ImageCreateInfo imageInfo{};
            imageInfo.imageType = vk::ImageType::e2D;
            imageInfo.format = vk::Format::eR8G8B8A8Unorm;
            imageInfo.extent = { desc.width, desc.height, 1 };
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = vk::SampleCountFlagBits::e1;
            imageInfo.tiling = vk::ImageTiling::eOptimal;
            imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            imageInfo.initialLayout = vk::ImageLayout::eUndefined;
            TextureResource texture;
            texture.image = vk::raii::Image(_device.Device(), imageInfo);
            const vk::MemoryRequirements requirements = texture.image.getMemoryRequirements();
            vk::MemoryAllocateInfo allocation{};
            allocation.allocationSize = requirements.size;
            allocation.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eDeviceLocal);
            texture.memory = vk::raii::DeviceMemory(_device.Device(), allocation);
            texture.image.bindMemory(*texture.memory, 0);

            SubmitImmediate([&](vk::raii::CommandBuffer& commands)
            {
                vk::ImageMemoryBarrier2 toTransfer{};
                toTransfer.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
                toTransfer.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
                toTransfer.oldLayout = vk::ImageLayout::eUndefined;
                toTransfer.newLayout = vk::ImageLayout::eTransferDstOptimal;
                toTransfer.image = *texture.image;
                toTransfer.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
                vk::DependencyInfo dependency{};
                dependency.imageMemoryBarrierCount = 1;
                dependency.pImageMemoryBarriers = &toTransfer;
                commands.pipelineBarrier2(dependency);

                vk::BufferImageCopy copy{};
                copy.imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
                copy.imageExtent = { desc.width, desc.height, 1 };
                commands.copyBufferToImage(*staging.buffer, *texture.image,
                    vk::ImageLayout::eTransferDstOptimal, copy);

                vk::ImageMemoryBarrier2 toShader{};
                toShader.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
                toShader.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
                toShader.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
                toShader.dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead;
                toShader.oldLayout = vk::ImageLayout::eTransferDstOptimal;
                toShader.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                toShader.image = *texture.image;
                toShader.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
                dependency.pImageMemoryBarriers = &toShader;
                commands.pipelineBarrier2(dependency);
            });

            vk::ImageViewCreateInfo viewInfo{};
            viewInfo.image = *texture.image;
            viewInfo.viewType = vk::ImageViewType::e2D;
            viewInfo.format = vk::Format::eR8G8B8A8Unorm;
            viewInfo.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
            texture.view = vk::raii::ImageView(_device.Device(), viewInfo);
            return _textures.Create(std::move(texture));
        }
        catch (const std::exception& error)
        {
            Debug::LogError("Vulkan texture creation failed: ", error.what());
            return {};
        }
    }

    GpuSamplerHandle VulkanGraphicsDevice::CreateSampler(const SamplerDesc& desc)
    {
        try
        {
            vk::SamplerCreateInfo info{};
            info.magFilter = desc.linear ? vk::Filter::eLinear : vk::Filter::eNearest;
            info.minFilter = info.magFilter;
            info.mipmapMode = vk::SamplerMipmapMode::eNearest;
            info.addressModeU = desc.repeat ? vk::SamplerAddressMode::eRepeat : vk::SamplerAddressMode::eClampToEdge;
            info.addressModeV = info.addressModeU;
            info.addressModeW = info.addressModeU;
            info.maxLod = 0.0f;
            return _samplers.Create(SamplerResource{ vk::raii::Sampler(_device.Device(), info) });
        }
        catch (...) { return {}; }
    }

    GpuShaderHandle VulkanGraphicsDevice::CreateShader(const ShaderProgramDesc& desc)
    {
        if (desc.vertexSpirv.empty() || desc.fragmentSpirv.empty())
        {
            Debug::LogError("Vulkan shader requires SPIR-V: ", desc.label);
            return {};
        }
        try
        {
            vk::ShaderModuleCreateInfo vertexInfo{};
            vertexInfo.codeSize = desc.vertexSpirv.size() * sizeof(uint32_t);
            vertexInfo.pCode = desc.vertexSpirv.data();
            vk::ShaderModuleCreateInfo fragmentInfo{};
            fragmentInfo.codeSize = desc.fragmentSpirv.size() * sizeof(uint32_t);
            fragmentInfo.pCode = desc.fragmentSpirv.data();
            ShaderResource resource;
            resource.vertex = vk::raii::ShaderModule(_device.Device(), vertexInfo);
            resource.fragment = vk::raii::ShaderModule(_device.Device(), fragmentInfo);
            return _shaders.Create(std::move(resource));
        }
        catch (...) { return {}; }
    }

    GpuPipelineHandle VulkanGraphicsDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
    {
        // bake shared shader, vertex, topology, and render state into a Vulkan pipeline
        ShaderResource* shader = _shaders.Get(desc.shader);
        if (!shader) return {};
        try
        {
            std::array<vk::PipelineShaderStageCreateInfo, 2> stages{};
            stages[0].stage = vk::ShaderStageFlagBits::eVertex;
            stages[0].module = *shader->vertex;
            stages[0].pName = "main";
            stages[1].stage = vk::ShaderStageFlagBits::eFragment;
            stages[1].module = *shader->fragment;
            stages[1].pName = "main";
            vk::VertexInputBindingDescription binding{};
            binding.binding = 0;
            binding.stride = desc.vertexLayout.stride;
            binding.inputRate = vk::VertexInputRate::eVertex;
            std::vector<vk::VertexInputAttributeDescription> attributes;
            for (const VertexAttribute& source : desc.vertexLayout.attributes)
            {
                vk::VertexInputAttributeDescription attribute{};
                attribute.location = source.location;
                attribute.binding = 0;
                attribute.format = ToVulkanFormat(source.type);
                attribute.offset = source.offset;
                attributes.push_back(attribute);
            }
            vk::PipelineVertexInputStateCreateInfo vertexInput{};
            vertexInput.vertexBindingDescriptionCount = 1;
            vertexInput.pVertexBindingDescriptions = &binding;
            vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
            vertexInput.pVertexAttributeDescriptions = attributes.data();
            vk::PipelineInputAssemblyStateCreateInfo assembly{};
            assembly.topology = ToVulkanTopology(desc.topology);
            vk::PipelineViewportStateCreateInfo viewport{};
            viewport.viewportCount = 1;
            viewport.scissorCount = 1;
            vk::PipelineRasterizationStateCreateInfo raster{};
            raster.polygonMode = vk::PolygonMode::eFill;
            raster.cullMode = desc.renderState.culling ? vk::CullModeFlagBits::eBack : vk::CullModeFlagBits::eNone;
            raster.frontFace = vk::FrontFace::eCounterClockwise;
            raster.lineWidth = 1.0f;
            vk::PipelineMultisampleStateCreateInfo multisample{};
            multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;
            vk::PipelineColorBlendAttachmentState blend{};
            blend.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            blend.blendEnable = desc.renderState.blendMode != BlendMode::NONE;
            switch (desc.renderState.blendMode)
            {
            case BlendMode::ADDITIVE:
                blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
                blend.dstColorBlendFactor = vk::BlendFactor::eOne;
                break;
            case BlendMode::PREMULTIPLIED_ALPHA:
                blend.srcColorBlendFactor = vk::BlendFactor::eOne;
                blend.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
                break;
            case BlendMode::MULTIPLY:
                blend.srcColorBlendFactor = vk::BlendFactor::eDstColor;
                blend.dstColorBlendFactor = vk::BlendFactor::eZero;
                break;
            default:
                blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
                blend.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
                break;
            }
            blend.colorBlendOp = vk::BlendOp::eAdd;
            blend.srcAlphaBlendFactor = blend.srcColorBlendFactor;
            blend.dstAlphaBlendFactor = blend.dstColorBlendFactor;
            blend.alphaBlendOp = vk::BlendOp::eAdd;
            vk::PipelineColorBlendStateCreateInfo blendState{};
            blendState.attachmentCount = 1;
            blendState.pAttachments = &blend;
            vk::PipelineDepthStencilStateCreateInfo depthState{};
            depthState.depthTestEnable = desc.renderState.depthTest;
            depthState.depthWriteEnable = desc.renderState.depthWrite;
            depthState.depthCompareOp = vk::CompareOp::eLess;
            std::array dynamicStates{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };
            vk::PipelineDynamicStateCreateInfo dynamic{};
            dynamic.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamic.pDynamicStates = dynamicStates.data();
            vk::PushConstantRange push{};
            push.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
            push.offset = 0;
            push.size = sizeof(VulkanDrawConstants);
            const vk::DescriptorSetLayout setLayout = *_textureSetLayout;
            vk::PipelineLayoutCreateInfo layoutInfo{};
            layoutInfo.setLayoutCount = 1;
            layoutInfo.pSetLayouts = &setLayout;
            layoutInfo.pushConstantRangeCount = 1;
            layoutInfo.pPushConstantRanges = &push;
            PipelineResource resource;
            resource.layout = vk::raii::PipelineLayout(_device.Device(), layoutInfo);
            vk::PipelineRenderingCreateInfo rendering{};
            const vk::Format colorFormat = _swapchain.Format();
            rendering.colorAttachmentCount = 1;
            rendering.pColorAttachmentFormats = &colorFormat;
            rendering.depthAttachmentFormat = _depthFormat;
            vk::GraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.pNext = &rendering;
            pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
            pipelineInfo.pStages = stages.data();
            pipelineInfo.pVertexInputState = &vertexInput;
            pipelineInfo.pInputAssemblyState = &assembly;
            pipelineInfo.pViewportState = &viewport;
            pipelineInfo.pRasterizationState = &raster;
            pipelineInfo.pMultisampleState = &multisample;
            pipelineInfo.pColorBlendState = &blendState;
            pipelineInfo.pDepthStencilState = &depthState;
            pipelineInfo.pDynamicState = &dynamic;
            pipelineInfo.layout = *resource.layout;
            resource.pipeline = vk::raii::Pipeline(_device.Device(), nullptr, pipelineInfo);
            return _pipelines.Create(std::move(resource));
        }
        catch (const std::exception& error)
        {
            Debug::LogError("Vulkan pipeline creation failed: ", error.what());
            return {};
        }
    }

    void VulkanGraphicsDevice::DestroyBuffer(GpuBufferHandle handle) { _buffers.Destroy(handle); }
    void VulkanGraphicsDevice::DestroyTexture(GpuTextureHandle handle)
    {
        for (auto set = _textureSets.begin(); set != _textureSets.end();)
        {
            if (set->first.texture == handle) set = _textureSets.erase(set);
            else ++set;
        }
        _textures.Destroy(handle);
    }

    void VulkanGraphicsDevice::DestroySampler(GpuSamplerHandle handle)
    {
        for (auto set = _textureSets.begin(); set != _textureSets.end();)
        {
            if (set->first.sampler == handle) set = _textureSets.erase(set);
            else ++set;
        }
        _samplers.Destroy(handle);
    }
    void VulkanGraphicsDevice::DestroyShader(GpuShaderHandle handle) { _shaders.Destroy(handle); }
    void VulkanGraphicsDevice::DestroyPipeline(GpuPipelineHandle handle) { _pipelines.Destroy(handle); }

    FrameStatus VulkanGraphicsDevice::BeginFrame(FrameContext&)
    {
        // wait for this frame slot before acquiring and resetting its command buffer
        _frameReady = false;
        try
        {
            if (!RecreateSwapchain()) return FrameStatus::Skip;
            VulkanFrameResources& frame = _frames[_frameIndex];
            if (_device.Device().waitForFences(*frame.inFlightFence, vk::True, UINT64_MAX) != vk::Result::eSuccess)
                return FrameStatus::Fatal;
            const auto [result, image] = _swapchain.Swapchain().acquireNextImage(UINT64_MAX, *frame.imageAvailable, nullptr);
            if (result == vk::Result::eErrorOutOfDateKHR) { _resizePending = true; return FrameStatus::ResizeRequired; }
            if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) return FrameStatus::Fatal;
            _resizePending |= result == vk::Result::eSuboptimalKHR;
            _imageIndex = image;
            _device.Device().resetFences(*frame.inFlightFence);
            frame.commandPool.reset({});
            frame.commandBuffer.begin({});
            _activePipeline = {};
            _frameReady = true;
            return FrameStatus::Success;
        }
        catch (const vk::SystemError& error)
        {
            Debug::LogError("Vulkan BeginFrame failed: ", error.what());
            return FromVulkanError(error);
        }
        catch (const std::exception& error)
        {
            Debug::LogError("Vulkan BeginFrame failed: ", error.what());
            return FrameStatus::Fatal;
        }
    }

    IGraphicsCommandList& VulkanGraphicsDevice::GetCommandList(FrameContext&) { return *this; }

    FrameStatus VulkanGraphicsDevice::EndFrame(FrameContext&)
    {
        // submit recorded commands and signal the semaphore used by presentation
        if (!_frameReady) return FrameStatus::Skip;
        try
        {
            VulkanFrameResources& frame = _frames[_frameIndex];
            frame.commandBuffer.end();
            vk::CommandBufferSubmitInfo commandInfo{};
            commandInfo.commandBuffer = *frame.commandBuffer;
            vk::SemaphoreSubmitInfo waitInfo{};
            waitInfo.semaphore = *frame.imageAvailable;
            waitInfo.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            vk::SemaphoreSubmitInfo signalInfo{};
            signalInfo.semaphore = *_renderFinished.at(_imageIndex);
            signalInfo.stageMask = vk::PipelineStageFlagBits2::eAllGraphics;
            vk::SubmitInfo2 submit{};
            submit.waitSemaphoreInfoCount = 1;
            submit.pWaitSemaphoreInfos = &waitInfo;
            submit.commandBufferInfoCount = 1;
            submit.pCommandBufferInfos = &commandInfo;
            submit.signalSemaphoreInfoCount = 1;
            submit.pSignalSemaphoreInfos = &signalInfo;
            _device.GraphicsQueue().submit2(submit, *frame.inFlightFence);
            return FrameStatus::Success;
        }
        catch (const vk::SystemError& error)
        {
            _frameReady = false;
            Debug::LogError("Vulkan EndFrame failed: ", error.what());
            return FromVulkanError(error);
        }
        catch (...)
        {
            _frameReady = false;
            return FrameStatus::Fatal;
        }
    }

    FrameStatus VulkanGraphicsDevice::Present(FrameContext&)
    {
        // present the acquired image and defer swapchain recreation to the next frame
        if (!_frameReady) return FrameStatus::Skip;
        try
        {
            const vk::SwapchainKHR swapchain = *_swapchain.Swapchain();
            const vk::Semaphore semaphore = *_renderFinished.at(_imageIndex);
            vk::PresentInfoKHR info{};
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &semaphore;
            info.swapchainCount = 1;
            info.pSwapchains = &swapchain;
            info.pImageIndices = &_imageIndex;
            const vk::Result result = _device.PresentQueue().presentKHR(info);
            _resizePending |= result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR;
            _frameReady = false;
            _frameIndex = (_frameIndex + 1) % FramesInFlight;
            if (_resizePending) return FrameStatus::ResizeRequired;
            return result == vk::Result::eSuccess ? FrameStatus::Success : FrameStatus::Fatal;
        }
        catch (const vk::SystemError& error)
        {
            _frameReady = false;
            Debug::LogError("Vulkan Present failed: ", error.what());
            return FromVulkanError(error);
        }
        catch (...)
        {
            _frameReady = false;
            return FrameStatus::Fatal;
        }
    }

    void VulkanGraphicsDevice::OnResize(uint32_t width, uint32_t height)
    {
        _requestedExtent = { width, height };
        _resizePending = true;
    }

    void VulkanGraphicsDevice::WaitIdle() { _device.WaitIdle(); }

    void VulkanGraphicsDevice::Shutdown()
    {
        try { WaitIdle(); } catch (...) {}
        _textureSets.clear();
        _pipelines.Clear();
        _shaders.Clear();
        _samplers.Clear();
        _textures.Clear();
        _buffers.Clear();
        _descriptorPool = nullptr;
        _textureSetLayout = nullptr;
        _renderFinished.clear();
        for (VulkanFrameResources& frame : _frames)
        {
            frame.commandBuffer = nullptr;
            frame.commandPool = nullptr;
            frame.inFlightFence = nullptr;
            frame.imageAvailable = nullptr;
        }
        _depthResources.clear();
        _swapchain.Shutdown();
        _device.Shutdown();
        _context.Shutdown();
        _window = nullptr;
        _depthFormat = vk::Format::eUndefined;
        _frameReady = false;
    }

    void VulkanGraphicsDevice::BeginRenderPass(const RenderPassDesc& desc)
    {
        // dynamic rendering uses the current swapchain view without a render-pass object
        vk::raii::CommandBuffer& commands = CurrentCommands();
        TransitionForRender(commands);
        vk::ClearValue clear{};
        clear.color.float32 = std::array{ desc.clearColor.r, desc.clearColor.g, desc.clearColor.b, desc.clearColor.a };
        vk::RenderingAttachmentInfo attachment{};
        attachment.imageView = _swapchain.ImageView(_imageIndex);
        attachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        attachment.loadOp = desc.clearColorTarget ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
        attachment.storeOp = vk::AttachmentStoreOp::eStore;
        attachment.clearValue = clear;
        vk::ClearValue depthClear{};
        depthClear.depthStencil = { 1.0f, 0 };
        vk::RenderingAttachmentInfo depthAttachment{};
        depthAttachment.imageView = *_depthResources.at(_imageIndex).view;
        depthAttachment.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        depthAttachment.loadOp = desc.clearDepthTarget ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        depthAttachment.clearValue = depthClear;
        vk::RenderingInfo info{};
        info.renderArea = { {0, 0}, _swapchain.Extent() };
        info.layerCount = 1;
        info.colorAttachmentCount = 1;
        info.pColorAttachments = &attachment;
        info.pDepthAttachment = &depthAttachment;
        commands.beginRendering(info);
        _renderPassActive = true;
    }

    void VulkanGraphicsDevice::EndRenderPass()
    {
        if (!_renderPassActive) return;
        CurrentCommands().endRendering();
        TransitionForPresent(CurrentCommands());
        _renderPassActive = false;
    }

    void VulkanGraphicsDevice::SetViewport(const ViewportDesc& viewport)
    {
        vk::Viewport native{ viewport.x, viewport.y + viewport.height, viewport.width, -viewport.height, 0.0f, 1.0f };
        vk::Rect2D scissor{ { static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y) },
            { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) } };
        CurrentCommands().setViewport(0, native);
        CurrentCommands().setScissor(0, scissor);
    }

    void VulkanGraphicsDevice::SetScissor(const ScissorDesc& scissor)
    {
        vk::Rect2D native{};
        native.offset = { scissor.x, scissor.y };
        native.extent = { scissor.width, scissor.height };
        CurrentCommands().setScissor(0, native);
    }

    void VulkanGraphicsDevice::SetPipeline(GpuPipelineHandle pipeline)
    {
        if (PipelineResource* resource = _pipelines.Get(pipeline))
        {
            _activePipeline = pipeline;
            CurrentCommands().bindPipeline(vk::PipelineBindPoint::eGraphics, *resource->pipeline);
        }
    }

    void VulkanGraphicsDevice::SetVertexBuffer(GpuBufferHandle buffer, const VertexLayout&)
    {
        if (BufferResource* resource = _buffers.Get(buffer))
        {
            const vk::Buffer native = *resource->buffer;
            const vk::DeviceSize offset = 0;
            CurrentCommands().bindVertexBuffers(0, native, offset);
        }
    }

    void VulkanGraphicsDevice::SetIndexBuffer(GpuBufferHandle buffer, IndexFormat)
    {
        if (BufferResource* resource = _buffers.Get(buffer))
            CurrentCommands().bindIndexBuffer(*resource->buffer, 0, vk::IndexType::eUint32);
    }

    void VulkanGraphicsDevice::SetFrameConstants(const FrameConstants& constants) { _frameConstants = constants; }

    void VulkanGraphicsDevice::SetMaterialResources(GpuTextureHandle texture, GpuSamplerHandle sampler)
    {
        // allocate one stable descriptor set for each texture and sampler pair
        TextureResource* image = _textures.Get(texture);
        SamplerResource* nativeSampler = _samplers.Get(sampler);
        PipelineResource* pipeline = _pipelines.Get(_activePipeline);
        if (!image || !nativeSampler || !pipeline) return;
        const TextureSetKey key{ texture, sampler };
        auto found = _textureSets.find(key);
        if (found == _textureSets.end())
        {
            vk::DescriptorSetAllocateInfo allocation{};
            allocation.descriptorPool = *_descriptorPool;
            const vk::DescriptorSetLayout layout = *_textureSetLayout;
            allocation.descriptorSetCount = 1;
            allocation.pSetLayouts = &layout;
            auto sets = _device.Device().allocateDescriptorSets(allocation);
            vk::raii::DescriptorSet set = std::move(sets.front());
            vk::DescriptorImageInfo imageInfo{ *nativeSampler->sampler, *image->view, vk::ImageLayout::eShaderReadOnlyOptimal };
            vk::WriteDescriptorSet write{};
            write.dstSet = *set;
            write.dstBinding = 0;
            write.descriptorCount = 1;
            write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            write.pImageInfo = &imageInfo;
            _device.Device().updateDescriptorSets(write, {});
            TextureSet textureSet;
            textureSet.set = std::move(set);
            found = _textureSets.emplace(key, std::move(textureSet)).first;
        }
        const vk::DescriptorSet set = *found->second.set;
        CurrentCommands().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline->layout, 0, set, {});
    }

    void VulkanGraphicsDevice::SetDrawConstants(const DrawConstants& constants)
    {
        // combine frame and model transforms before writing the push constants
        PipelineResource* pipeline = _pipelines.Get(_activePipeline);
        if (!pipeline) return;
        VulkanDrawConstants native;
        native.mvp = _frameConstants.projection * _frameConstants.view * constants.model;
        native.tint = constants.tint;
        native.uvRect = constants.uvRect;
        CurrentCommands().pushConstants<VulkanDrawConstants>(*pipeline->layout,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, native);
    }

    void VulkanGraphicsDevice::Draw(uint32_t vertexCount)
    {
        if (_renderPassActive && _activePipeline) CurrentCommands().draw(vertexCount, 1, 0, 0);
    }

    void VulkanGraphicsDevice::DrawIndexed(uint32_t indexCount)
    {
        if (_renderPassActive && _activePipeline) CurrentCommands().drawIndexed(indexCount, 1, 0, 0, 0);
    }

    void VulkanGraphicsDevice::AddDebugMarker(const char*)
    {
        // Vulkan debug labels require an optional device extension; shared labels stay backend-neutral
    }

    uint32_t VulkanGraphicsDevice::FindMemoryType(uint32_t bits, vk::MemoryPropertyFlags properties) const
    {
        const vk::PhysicalDeviceMemoryProperties memory = _device.PhysicalDevice().getMemoryProperties();
        for (uint32_t index = 0; index < memory.memoryTypeCount; ++index)
            if ((bits & (1u << index)) && (memory.memoryTypes[index].propertyFlags & properties) == properties) return index;
        throw std::runtime_error("No compatible Vulkan memory type");
    }

    vk::Format VulkanGraphicsDevice::FindDepthFormat() const
    {
        for (const vk::Format format : { vk::Format::eD32Sfloat, vk::Format::eD16Unorm })
        {
            const vk::FormatProperties properties = _device.PhysicalDevice().getFormatProperties(format);
            if (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
                return format;
        }
        throw std::runtime_error("No supported Vulkan depth format");
    }

    VulkanGraphicsDevice::BufferResource VulkanGraphicsDevice::CreateBufferResource(
        const void* data, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
    {
        BufferResource resource;
        vk::BufferCreateInfo info{};
        info.size = size;
        info.usage = usage;
        info.sharingMode = vk::SharingMode::eExclusive;
        resource.buffer = vk::raii::Buffer(_device.Device(), info);
        const vk::MemoryRequirements requirements = resource.buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo allocation{};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, properties);
        resource.memory = vk::raii::DeviceMemory(_device.Device(), allocation);
        resource.buffer.bindMemory(*resource.memory, 0);
        if (data)
        {
            void* destination = resource.memory.mapMemory(0, size);
            std::memcpy(destination, data, size);
            resource.memory.unmapMemory();
        }
        resource.size = size;
        return resource;
    }

    void VulkanGraphicsDevice::SubmitImmediate(const std::function<void(vk::raii::CommandBuffer&)>& record)
    {
        // short upload work uses a temporary pool and waits before temporary owners expire
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
        poolInfo.queueFamilyIndex = _device.GraphicsQueueFamily();
        vk::raii::CommandPool pool(_device.Device(), poolInfo);
        vk::CommandBufferAllocateInfo allocation{};
        allocation.commandPool = *pool;
        allocation.level = vk::CommandBufferLevel::ePrimary;
        allocation.commandBufferCount = 1;
        auto buffers = _device.Device().allocateCommandBuffers(allocation);
        vk::raii::CommandBuffer& commands = buffers.front();
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        commands.begin(beginInfo);
        record(commands);
        commands.end();
        vk::CommandBufferSubmitInfo commandInfo{};
        commandInfo.commandBuffer = *commands;
        vk::SubmitInfo2 submit{};
        submit.commandBufferInfoCount = 1;
        submit.pCommandBufferInfos = &commandInfo;
        _device.GraphicsQueue().submit2(submit);
        _device.GraphicsQueue().waitIdle();
    }

    void VulkanGraphicsDevice::CreateFrameResources()
    {
        for (VulkanFrameResources& frame : _frames)
        {
            vk::CommandPoolCreateInfo poolInfo{};
            poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            poolInfo.queueFamilyIndex = _device.GraphicsQueueFamily();
            frame.commandPool = vk::raii::CommandPool(_device.Device(), poolInfo);
            vk::CommandBufferAllocateInfo allocation{};
            allocation.commandPool = *frame.commandPool;
            allocation.level = vk::CommandBufferLevel::ePrimary;
            allocation.commandBufferCount = 1;
            auto buffers = _device.Device().allocateCommandBuffers(allocation);
            frame.commandBuffer = std::move(buffers.front());
            vk::FenceCreateInfo fenceInfo{};
            fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
            frame.inFlightFence = vk::raii::Fence(_device.Device(), fenceInfo);
            frame.imageAvailable = vk::raii::Semaphore(_device.Device(), vk::SemaphoreCreateInfo{});
        }
        _renderFinished.clear();
        for (uint32_t index = 0; index < _swapchain.ImageCount(); ++index)
            _renderFinished.emplace_back(_device.Device(), vk::SemaphoreCreateInfo{});
    }

    void VulkanGraphicsDevice::CreateDepthResources()
    {
        // keep one depth image per swapchain image so frames in flight never share it
        std::vector<DepthResource> replacements;
        replacements.reserve(_swapchain.ImageCount());
        for (uint32_t index = 0; index < _swapchain.ImageCount(); ++index)
        {
            vk::ImageCreateInfo imageInfo{};
            imageInfo.imageType = vk::ImageType::e2D;
            imageInfo.format = _depthFormat;
            imageInfo.extent = { _swapchain.Extent().width, _swapchain.Extent().height, 1 };
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = vk::SampleCountFlagBits::e1;
            imageInfo.tiling = vk::ImageTiling::eOptimal;
            imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
            imageInfo.initialLayout = vk::ImageLayout::eUndefined;

            DepthResource depth;
            depth.image = vk::raii::Image(_device.Device(), imageInfo);
            const vk::MemoryRequirements requirements = depth.image.getMemoryRequirements();
            vk::MemoryAllocateInfo allocation{};
            allocation.allocationSize = requirements.size;
            allocation.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eDeviceLocal);
            depth.memory = vk::raii::DeviceMemory(_device.Device(), allocation);
            depth.image.bindMemory(*depth.memory, 0);

            vk::ImageViewCreateInfo viewInfo{};
            viewInfo.image = *depth.image;
            viewInfo.viewType = vk::ImageViewType::e2D;
            viewInfo.format = _depthFormat;
            viewInfo.subresourceRange = { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };
            depth.view = vk::raii::ImageView(_device.Device(), viewInfo);
            replacements.push_back(std::move(depth));
        }

        SubmitImmediate([&](vk::raii::CommandBuffer& commands)
        {
            std::vector<vk::ImageMemoryBarrier2> barriers;
            barriers.reserve(replacements.size());
            for (const DepthResource& depth : replacements)
            {
                vk::ImageMemoryBarrier2 barrier{};
                barrier.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                    vk::PipelineStageFlagBits2::eLateFragmentTests;
                barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                    vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
                barrier.oldLayout = vk::ImageLayout::eUndefined;
                barrier.newLayout = vk::ImageLayout::eDepthAttachmentOptimal;
                barrier.image = *depth.image;
                barrier.subresourceRange = { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };
                barriers.push_back(barrier);
            }
            vk::DependencyInfo dependency{};
            dependency.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
            dependency.pImageMemoryBarriers = barriers.data();
            commands.pipelineBarrier2(dependency);
        });

        _depthResources = std::move(replacements);
    }

    void VulkanGraphicsDevice::CreateDescriptors()
    {
        vk::DescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding.descriptorCount = 1;
        binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        _textureSetLayout = vk::raii::DescriptorSetLayout(_device.Device(), layoutInfo);
        vk::DescriptorPoolSize size{};
        size.type = vk::DescriptorType::eCombinedImageSampler;
        size.descriptorCount = 2048;
        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = 2048;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &size;
        _descriptorPool = vk::raii::DescriptorPool(_device.Device(), poolInfo);
    }

    bool VulkanGraphicsDevice::RecreateSwapchain()
    {
        // skip recreation while minimized and rebuild image-count-dependent semaphores
        if (!_resizePending) return true;
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(_window, &width, &height);
        if (width <= 0 || height <= 0) return false;
        WaitIdle();
        _requestedExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        if (!_swapchain.Recreate(_device, _context.SurfaceHandle(), _requestedExtent)) return false;
        CreateDepthResources();
        _renderFinished.clear();
        for (uint32_t index = 0; index < _swapchain.ImageCount(); ++index)
            _renderFinished.emplace_back(_device.Device(), vk::SemaphoreCreateInfo{});
        _resizePending = false;
        return true;
    }

    vk::raii::CommandBuffer& VulkanGraphicsDevice::CurrentCommands() { return _frames[_frameIndex].commandBuffer; }

    void VulkanGraphicsDevice::TransitionForRender(vk::raii::CommandBuffer& commands)
    {
        vk::ImageMemoryBarrier2 barrier{};
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        barrier.image = _swapchain.Image(_imageIndex);
        barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
        vk::DependencyInfo dependency{};
        dependency.imageMemoryBarrierCount = 1;
        dependency.pImageMemoryBarriers = &barrier;
        commands.pipelineBarrier2(dependency);
    }

    void VulkanGraphicsDevice::TransitionForPresent(vk::raii::CommandBuffer& commands)
    {
        vk::ImageMemoryBarrier2 barrier{};
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
        barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
        barrier.image = _swapchain.Image(_imageIndex);
        barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
        vk::DependencyInfo dependency{};
        dependency.imageMemoryBarrierCount = 1;
        dependency.pImageMemoryBarriers = &barrier;
        commands.pipelineBarrier2(dependency);
    }
}
