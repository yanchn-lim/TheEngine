#pragma once

#include <cstdint>

namespace Assets
{
	using AssetId = uint32_t;

	constexpr AssetId InvalidAssetId = 0;

	struct TextureHandle
	{
		AssetId id = InvalidAssetId;

		bool IsValid() const
		{
			return id != InvalidAssetId;
		}

		explicit operator bool() const
		{
			return IsValid();
		}
	};

	struct ShaderHandle
	{
		AssetId id = InvalidAssetId;

		bool IsValid() const
		{
			return id != InvalidAssetId;
		}

		explicit operator bool() const
		{
			return IsValid();
		}
	};

	struct MeshHandle
	{
		AssetId id = InvalidAssetId;

		bool IsValid() const
		{
			return id != InvalidAssetId;
		}

		explicit operator bool() const
		{
			return IsValid();
		}
	};

	struct MaterialHandle
	{
		AssetId id = InvalidAssetId;

		bool IsValid() const
		{
			return id != InvalidAssetId;
		}

		explicit operator bool() const
		{
			return IsValid();
		}
	};

	struct ModelHandle
	{
		AssetId id = InvalidAssetId;

		bool IsValid() const
		{
			return id != InvalidAssetId;
		}

		explicit operator bool() const
		{
			return IsValid();
		}
	};
}
