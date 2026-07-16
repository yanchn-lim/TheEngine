# Buffers and Meshes

[[Vulkan Implementation Tutorial|Back to tutorial index]]

## Goal

Move existing backend-neutral `MeshUploadData` into Vulkan vertex and index buffers. `VertexArray` remains OpenGL-only; Vulkan describes vertex input when it creates a graphics pipeline and binds buffers directly during command recording.

```text
Assets importer / Primitive2D
    -> MeshUploadData
    -> VulkanBuffer uploads
    -> VulkanMesh
    -> DrawCmd resolves VulkanMesh
```

## Buffer Ownership

Start with `VulkanBuffer` as a move-only owner of one `VkBuffer`, its allocation, and its size. The allocation implementation can be direct Vulkan memory now; keep its public shape independent enough that Vulkan Memory Allocator can replace the internals later.

```cpp
class VulkanBuffer
{
public:
    bool Create(const VulkanDevice& device, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
    void Shutdown(const VulkanDevice& device);

    VkBuffer GetHandle() const { return _buffer; }
    VkDeviceSize GetSize() const { return _size; }

private:
    VkBuffer _buffer{ VK_NULL_HANDLE };
    VkDeviceMemory _memory{ VK_NULL_HANDLE };
    VkDeviceSize _size = 0;
};
```

Create the buffer, query memory requirements, select a compatible memory type, allocate memory, and bind it:

```cpp
VkBufferCreateInfo bufferInfo{};
bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
bufferInfo.size = size;
bufferInfo.usage = usage;
bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
VK_CHECK(vkCreateBuffer(device.GetHandle(), &bufferInfo, nullptr, &_buffer));

VkMemoryRequirements requirements{};
vkGetBufferMemoryRequirements(device.GetHandle(), _buffer, &requirements);

VkMemoryAllocateInfo allocationInfo{};
allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
allocationInfo.allocationSize = requirements.size;
allocationInfo.memoryTypeIndex = FindMemoryType(
    device.GetPhysicalDevice(), requirements.memoryTypeBits, memoryProperties);
VK_CHECK(vkAllocateMemory(device.GetHandle(), &allocationInfo, nullptr, &_memory));
VK_CHECK(vkBindBufferMemory(device.GetHandle(), _buffer, _memory, 0));
```

`FindMemoryType` scans `vkGetPhysicalDeviceMemoryProperties` and returns a memory-type index whose bit is set in `typeFilter` and whose properties contain every requested flag.

## Staging Uploads

GPU-local memory is normally not CPU-visible. Upload through a host-visible staging buffer:

```text
CPU MeshUploadData bytes
    -> host-visible, host-coherent staging buffer
    -> vkCmdCopyBuffer
    -> device-local vertex/index buffer
```

```cpp
void VulkanBuffer::Write(const VulkanDevice& device, const void* data, VkDeviceSize size)
{
    void* mapped = nullptr;
    VK_CHECK(vkMapMemory(device.GetHandle(), _memory, 0, size, 0, &mapped));
    std::memcpy(mapped, data, static_cast<size_t>(size));
    vkUnmapMemory(device.GetHandle(), _memory);
}
```

Use `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` for the staging buffer and `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` for the final buffer. A non-coherent staging path needs `vkFlushMappedMemoryRanges`; do not assume all host-visible memory is coherent.

Record a copy in a one-time command buffer, submit it, and wait before destroying the staging buffer in this first version:

```cpp
VkBufferCopy copy{};
copy.size = byteSize;
vkCmdCopyBuffer(commandBuffer, staging.GetHandle(), destination.GetHandle(), 1, &copy);
```

Later, replace the wait with a transfer queue or deferred upload-deletion queue. Do not optimize it before basic mesh rendering is correct.

## Vertex Input

Translate the existing `VertexLayout` into `VkVertexInputBindingDescription` and `VkVertexInputAttributeDescription` when the pipeline is created. The pipeline must match both the CPU structure and shader `layout(location = N)` declarations.

```cpp
VkVertexInputBindingDescription binding{};
binding.binding = 0;
binding.stride = static_cast<uint32_t>(layout.GetStride());
binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

std::vector<VkVertexInputAttributeDescription> attributes;
for (const Graphics::VertexAttribute& attribute : layout.GetAttributes())
{
    VkVertexInputAttributeDescription description{};
    description.location = attribute.location;
    description.binding = 0;
    description.format = ToVkFormat(attribute.type);
    description.offset = attribute.offset;
    attributes.push_back(description);
}
```

The standard mesh vertex format may include position, color, UV, normal, and tangent, but only enable locations the pipeline/shader uses. Missing attributes are not represented by gaps in a bound buffer; either use a different compatible layout/pipeline or provide default data in the vertex stream.

## Vulkan Mesh and Draw

```cpp
class VulkanMesh
{
public:
    bool Create(const Graphics::MeshUploadData& source);
    void Bind(VkCommandBuffer commandBuffer) const;
    uint32_t GetIndexCount() const { return _indexCount; }

private:
    VulkanBuffer _vertexBuffer;
    VulkanBuffer _indexBuffer;
    uint32_t _indexCount = 0;
};

void VulkanMesh::Bind(VkCommandBuffer commandBuffer) const
{
    const VkBuffer vertexBuffers[] = { _vertexBuffer.GetHandle() };
    const VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _indexBuffer.GetHandle(), 0, VK_INDEX_TYPE_UINT32);
}
```

Inside dynamic rendering, bind the pipeline, mesh buffers, then call `vkCmdDrawIndexed(commandBuffer, mesh.GetIndexCount(), 1, 0, 0, 0)`.

## Verify

Convert the existing indexed quad first, then a `Primitive2D` shape, then an imported OBJ mesh. Compare them to the OpenGL output. Incorrect stride, offset, format, index type, or shader location usually produces distorted geometry rather than a Vulkan error.

Next: [[08 - Descriptors Camera and Materials]].
