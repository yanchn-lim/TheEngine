#include "asset_types.hpp"

namespace Asset
{

}

std::ostream& operator<< (std::ostream& os, Asset::AssetHandle handle)
{
#ifdef _DEBUG
	os << handle.name;
#else
	os << handle.id;
#endif
	return os;
}