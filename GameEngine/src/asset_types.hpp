#pragma once

namespace Asset
{
	constexpr uint Hash(const char* str) //FNV-1a hash
	{
		uint32_t hash = 2166136261u; // FNV offset basis

		while (*str)
		{
			hash ^= static_cast<uint32_t>(*str++);
			hash *= 16777619u; // FNV prime
		}

		return hash;
	}

	struct AssetHandle
	{
		uint id{ 0 }; //hashed id from a string
#ifdef _DEBUG
		std::string name{}; //for debugging and pinpointing missing assets
#endif

		explicit operator bool() const { return id != 0; }
		bool operator==(const AssetHandle& o) const { return id == o.id; }
		bool operator!=(const AssetHandle& o) const { return !(id == o.id); }
		bool operator< (const AssetHandle& o) const { return id < o.id; }

		AssetHandle() : id(0) {}

		AssetHandle(const char* nm) : id(Hash(nm))
		{
#ifdef _DEBUG
			name = nm;
#endif
		}
	};
}

namespace std
{
	//give std::hash a way to hash AssetHandle
	template<>
	struct hash<Asset::AssetHandle>
	{
		size_t operator()(const Asset::AssetHandle& h) const noexcept
		{
			return std::hash<uint>{}(h.id);
		}
	};
}

std::ostream& operator<< (std::ostream& os, Asset::AssetHandle handle);
