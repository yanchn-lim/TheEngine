#include <filesystem>
#include <algorithm>
#include <cctype>

#include "obj_model_importer.hpp"

namespace Assets
{
	bool ObjModelImporter::CanLoad(const std::string& path) const
	{
		//get extension
		std::string ext = std::filesystem::path(path).extension().string();

		//to lowercase
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		//check
		return ext == ".obj";
	}

	bool ObjModelImporter::Load(const std::string& /*path*/, MeshSourceCollection& /*outModel*/)
	{
		//impl tinyobj here
		return false;
	}
}