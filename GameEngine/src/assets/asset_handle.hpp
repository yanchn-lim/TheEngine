#pragma once

#include <cstdint>

namespace Ludus::Assets
{
	// asset ids identify cpu records and remain independent from gpu handles.
	using AssetId = uint32_t;

	constexpr AssetId InvalidAssetId = 0;

	// distinct handle types prevent accidental cross-registry lookups.
	// handles borrow records and become stale when their registry is cleared.
	// id reuse means that an old handle can later identify a new record.
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
