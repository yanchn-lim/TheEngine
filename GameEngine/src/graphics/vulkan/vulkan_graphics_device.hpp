#pragma once

#include <unordered_map>

#include "graphics/graphics_device.hpp"
#include "graphics/resource_table.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"
#include "vulkan_frameresources.hpp"
#include "vulkan_swapchain.hpp"

namespace Graphics
{
    // implements the shared device and command interfaces with Vulkan recording
    class VulkanGraphicsDevice final : public IGraphicsDevice, public IGraphicsCommandList
    {
    public:
        bool Initialize(const GraphicsDeviceDesc& desc) override;
        const GraphicsCapabilities& GetCapabilities() const override { return _capabilities; }
        GpuBufferHandle CreateBuffer(const BufferDesc& desc) override;
        GpuTextureHandle CreateTexture(const TextureDesc& desc) override;
        GpuSamplerHandle CreateSampler(const SamplerDesc& desc) override;
        GpuShaderHandle CreateShader(const ShaderProgramDesc& desc) override;
        GpuPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
        void DestroyBuffer(GpuBufferHandle handle) override;
        void DestroyTexture(GpuTextureHandle handle) override;
        void DestroySampler(GpuSamplerHandle handle) override;
        void DestroyShader(GpuShaderHandle handle) override;
        void DestroyPipeline(GpuPipelineHandle handle) override;
        FrameStatus BeginFrame(FrameContext& context) override;
        IGraphicsCommandList& GetCommandList(FrameContext& context) override;
        FrameStatus EndFrame(FrameContext& context) override;
        FrameStatus Present(FrameContext& context) override;
        void OnResize(uint32_t width, uint32_t height) override;
        void WaitIdle() override;
        void Shutdown() override;

        void BeginRenderPass(const RenderPassDesc& desc) override;
        void EndRenderPass() override;
        void SetViewport(const ViewportDesc& viewport) override;
        void SetScissor(const ScissorDesc& scissor) override;
        void SetPipeline(GpuPipelineHandle pipeline) override;
        void SetVertexBuffer(GpuBufferHandle buffer, const VertexLayout& layout) override;
        void SetIndexBuffer(GpuBufferHandle buffer, IndexFormat format) override;
        void SetFrameConstants(const FrameConstants& constants) override;
        void SetMaterialResources(GpuTextureHandle texture, GpuSamplerHandle sampler) override;
        void SetDrawConstants(const DrawConstants& constants) override;
        void Draw(uint32_t vertexCount) override;
        void DrawIndexed(uint32_t indexCount) override;
        void AddDebugMarker(const char* label) override;

        vk::Instance NativeInstance() const { return *_context.Instance(); }
        vk::PhysicalDevice NativePhysicalDevice() const { return *_device.PhysicalDevice(); }
        vk::Device NativeDevice() const { return *_device.Device(); }
        vk::Queue NativeGraphicsQueue() const { return *_device.GraphicsQueue(); }
        uint32_t NativeGraphicsQueueFamily() const { return _device.GraphicsQueueFamily(); }
        vk::Format SwapchainFormat() const { return _swapchain.Format(); }
        uint32_t SwapchainImageCount() const { return _swapchain.ImageCount(); }
        vk::CommandBuffer ActiveCommandBuffer() const { return *_frames[_frameIndex].commandBuffer; }

    private:
        // RAII entries keep Vulkan object destruction ordered inside each handle slot
        struct BufferResource
        {
            vk::raii::DeviceMemory memory{ nullptr };
            vk::raii::Buffer buffer{ nullptr };
            vk::DeviceSize size = 0;
        };
        struct TextureResource
        {
            vk::raii::DeviceMemory memory{ nullptr };
            vk::raii::Image image{ nullptr };
            vk::raii::ImageView view{ nullptr };
        };
        struct DepthResource
        {
            vk::raii::DeviceMemory memory{ nullptr };
            vk::raii::Image image{ nullptr };
            vk::raii::ImageView view{ nullptr };
        };
        struct SamplerResource { vk::raii::Sampler sampler{ nullptr }; };
        struct ShaderResource
        {
            vk::raii::ShaderModule vertex{ nullptr };
            vk::raii::ShaderModule fragment{ nullptr };
        };
        struct PipelineResource
        {
            vk::raii::PipelineLayout layout{ nullptr };
            vk::raii::Pipeline pipeline{ nullptr };
        };
        struct TextureSetKey
        {
            GpuTextureHandle texture;
            GpuSamplerHandle sampler;
            bool operator==(const TextureSetKey& other) const
            {
                return texture == other.texture && sampler == other.sampler;
            }
        };
        struct TextureSetKeyHash
        {
            size_t operator()(const TextureSetKey& key) const;
        };
        struct TextureSet
        {
            vk::raii::DescriptorSet set{ nullptr };
        };

        // context, device, swapchain, and frame owners follow Vulkan dependency order
        GLFWwindow* _window = nullptr;
        GraphicsCapabilities _capabilities{ "Vulkan 1.3", true };
        VulkanContext _context;
        VulkanDevice _device;
        VulkanSwapchain _swapchain;
        std::vector<DepthResource> _depthResources;
        std::array<VulkanFrameResources, FramesInFlight> _frames;
        std::vector<vk::raii::Semaphore> _renderFinished;
        vk::raii::DescriptorSetLayout _textureSetLayout{ nullptr };
        vk::raii::DescriptorPool _descriptorPool{ nullptr };
        std::unordered_map<TextureSetKey, TextureSet, TextureSetKeyHash> _textureSets;
        // resource tables hide native handles and reject stale generations
        ResourceTable<GpuBufferHandle, BufferResource> _buffers;
        ResourceTable<GpuTextureHandle, TextureResource> _textures;
        ResourceTable<GpuSamplerHandle, SamplerResource> _samplers;
        ResourceTable<GpuShaderHandle, ShaderResource> _shaders;
        ResourceTable<GpuPipelineHandle, PipelineResource> _pipelines;
        GpuPipelineHandle _activePipeline;
        FrameConstants _frameConstants;
        uint32_t _frameIndex = 0;
        uint32_t _imageIndex = 0;
        vk::Extent2D _requestedExtent{};
        vk::Format _depthFormat = vk::Format::eUndefined;
        bool _frameReady = false;
        bool _renderPassActive = false;
        bool _resizePending = false;

        // helpers own allocation, upload, descriptor, and layout-transition details
        uint32_t FindMemoryType(uint32_t bits, vk::MemoryPropertyFlags properties) const;
        vk::Format FindDepthFormat() const;
        BufferResource CreateBufferResource(const void* data, size_t size, vk::BufferUsageFlags usage,
            vk::MemoryPropertyFlags properties);
        void SubmitImmediate(const std::function<void(vk::raii::CommandBuffer&)>& record);
        void CreateFrameResources();
        void CreateDepthResources();
        void CreateDescriptors();
        bool RecreateSwapchain();
        vk::raii::CommandBuffer& CurrentCommands();
        void TransitionForRender(vk::raii::CommandBuffer& commands);
        void TransitionForPresent(vk::raii::CommandBuffer& commands);
    };
}
