#pragma once


namespace Graphics
{
	class VertexArray
	{
	private:
		uint32_t _id{ 0 };

	public:
		void Bind() const;
		void Destroy();
		uint32_t GetId() const;
	};
}