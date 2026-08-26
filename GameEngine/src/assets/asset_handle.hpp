#pragma once

#include <cstdint>

namespace Assets
{
	// asset ids identify CPU-side records and remain independent from GPU handles
	using AssetId = uint32_t;

	constexpr AssetId InvalidAssetId = 0;

	// distinct handle types prevent accidental cross-registry lookups
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

}
