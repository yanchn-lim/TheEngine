#pragma once


namespace Graphics
{
	class IndexBuffer
	{
	private:
		uint32_t _id;
		uint32_t _count;

	public:
		bool Create(const uint32_t* indices, uint32_t count);
		void Bind() const;
		void Destroy();
		bool IsValid() const;
		uint32_t GetCount() const;
		uint32_t GetId() const;

		IndexBuffer() = default;
		~IndexBuffer();

		IndexBuffer(const IndexBuffer&) = delete;
		IndexBuffer& operator=(const IndexBuffer&) = delete;
		IndexBuffer(IndexBuffer&&) noexcept;
		IndexBuffer& operator=(IndexBuffer&&) noexcept;
	};
}