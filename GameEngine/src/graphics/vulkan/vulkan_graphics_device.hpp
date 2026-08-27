#pragma once

#include <unordered_map>

#include "graphics/graphics_device.hpp"
#include "graphics/resource_table.hpp"
#include "vulkan_context.hpp"
#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"

namespace Ludus::Graphics
{
    // implements the shared graphics device interface with Vulkan recording
    class VulkanGraphicsDevice final : public IGraphicsDevice
    {
    public:
        bool Initialize(const GraphicsDeviceDesc& desc) override;
        GpuBufferHandle CreateBuffer(const BufferDesc& desc) override;
        GpuTextureHandle CreateTexture(const TextureDesc& desc) override;
        GpuSamplerHandle CreateSampler(const SamplerDesc& desc) override;
        GpuShaderHandle CreateShader(const ShaderProgramDesc& desc) override;
        GpuPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
		GpuRenderTargetHandle CreateRenderTarget(const RenderTargetDesc& desc) override;
		GpuTextureHandle GetRenderTargetTexture(GpuRenderTargetHandle handle) const override;
        void DestroyBuffer(GpuBufferHandle handle) override;
        void DestroyTexture(GpuTextureHandle handle) override;
        void DestroySampler(GpuSamplerHandle handle) override;
        void DestroyShader(GpuShaderHandle handle) override;
        void DestroyPipeline(GpuPipelineHandle handle) override;
		void DestroyRenderTarget(GpuRenderTargetHandle handle) override;
        FrameStatus BeginFrame() override;
        FrameStatus EndFrame() override;
        void OnResize(uint32_t width, uint32_t height) override;
        void WaitIdle() override;
        void Shutdown() override;

        void BeginRenderPass(const RenderPassDesc& desc) override;
        void EndRenderPass() override;
        void SetViewport(const ViewportDesc& viewport) override;
        void SetPipeline(GpuPipelineHandle pipeline) override;
        void SetVertexBuffer(GpuBufferHandle buffer, const VertexLayout& layout) override;
        void SetIndexBuffer(GpuBufferHandle buffer) override;
        void SetFrameConstants(const FrameConstants& constants) override;
        void SetMaterialResources(GpuTextureHandle texture, GpuSamplerHandle sampler) override;
        void SetDrawConstants(const DrawConstants& constants) override;
        void Draw(uint32_t vertexCount) override;
        void DrawIndexed(uint32_t indexCount) override;

        vk::Instance NativeInstance() const { return *_context.Instance(); }
        vk::PhysicalDevice NativePhysicalDevice() const { return *_device.PhysicalDevice(); }
        vk::Device NativeDevice() const { return *_device.Device(); }
        vk::Queue NativeGraphicsQueue() const { return *_device.GraphicsQueue(); }
        uint32_t NativeGraphicsQueueFamily() const { return _device.GraphicsQueueFamily(); }
        vk::Format SwapchainFormat() const { return _swapchain.Format(); }
        vk::Format DepthFormat() const { return _depthFormat; }
        uint32_t SwapchainImageCount() const { return _swapchain.ImageCount(); }
        vk::CommandBuffer ActiveCommandBuffer() const { return *_frames[_frameIndex].commandBuffer; }
		vk::ImageView NativeTextureView(GpuTextureHandle handle) const;
		vk::Sampler NativeSampler(GpuSamplerHandle handle) const;

    private:
        FrameStatus Present();

        static constexpr uint32_t FramesInFlight = 2;
        struct FrameResources
        {
            vk::raii::CommandPool commandPool{ nullptr };
            vk::raii::CommandBuffer commandBuffer{ nullptr };
            vk::raii::Fence inFlightFence{ nullptr };
            vk::raii::Semaphore imageAvailable{ nullptr };
        };

        // RAII entries keep Vulkan object destruction ordered inside each handle slot
        struct BufferResource
        {
            vk::raii::DeviceMemory memory{ nullptr };
            vk::raii::Buffer buffer{ nullptr };
        };
        struct ImageResource
        {
            vk::raii::DeviceMemory memory{ nullptr };
            vk::raii::Image image{ nullptr };
            vk::raii::ImageView view{ nullptr };
        };
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
		struct RenderTargetResource
		{
			GpuTextureHandle color;
			ImageResource depth;
			vk::Extent2D extent;
			bool rendered = false;
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
        // context, device, swapchain, and frame owners follow Vulkan dependency order
        GLFWwindow* _window = nullptr;
        VulkanContext _context;
        VulkanDevice _device;
        VulkanSwapchain _swapchain;
        std::vector<ImageResource> _depthResources;
        std::array<FrameResources, FramesInFlight> _frames;
        std::vector<vk::raii::Semaphore> _renderFinished;
        vk::raii::DescriptorSetLayout _textureSetLayout{ nullptr };
        vk::raii::DescriptorPool _descriptorPool{ nullptr };
        std::unordered_map<TextureSetKey, vk::raii::DescriptorSet, TextureSetKeyHash> _textureSets;
        // resource tables hide native handles and reject stale generations
        ResourceTable<GpuBufferHandle, BufferResource> _buffers;
        ResourceTable<GpuTextureHandle, ImageResource> _textures;
        ResourceTable<GpuSamplerHandle, vk::raii::Sampler> _samplers;
        ResourceTable<GpuShaderHandle, ShaderResource> _shaders;
        ResourceTable<GpuPipelineHandle, PipelineResource> _pipelines;
		ResourceTable<GpuRenderTargetHandle, RenderTargetResource> _renderTargets;
        GpuPipelineHandle _activePipeline;
        FrameConstants _frameConstants;
        uint32_t _frameIndex = 0;
        uint32_t _imageIndex = 0;
        vk::Format _depthFormat = vk::Format::eUndefined;
        bool _frameReady = false;
        bool _renderPassActive = false;
		GpuRenderTargetHandle _activeRenderTarget;
        bool _resizePending = false;
        bool _vsync = false;

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
