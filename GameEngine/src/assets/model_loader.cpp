#include "model_loader.hpp"

namespace Assets
{
	void ModelLoader::RegisterImporter(std::unique_ptr<IModelImporter> importer)
	{
		if (!importer)
			return;

		_importers.push_back(std::move(importer));
	}

	bool ModelLoader::Load(const std::string& path, MeshSourceCollection& outModel) const
	{
		//find the importer that support the format
		for (const auto& importer : _importers)
		{
			if (importer->CanLoad(path))
				return importer->Load(path, outModel);
		}

		return false;
	}
}