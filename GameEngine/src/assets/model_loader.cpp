#include "model_loader.hpp"

namespace Assets
{
	void ModelLoader::RegisterImporter(std::unique_ptr<IModelImporter> importer)
	{
		// Importers are tried in registration order when loading model files.
		if (!importer)
			return;

		_importers.push_back(std::move(importer));
	}

	bool ModelLoader::Load(const std::string& path, MeshSourceCollection& outModel) const
	{
		// Find the first importer that claims support for this file.
		for (const auto& importer : _importers)
		{
			if (importer->CanLoad(path))
				return importer->Load(path, outModel);
		}

		return false;
	}
}
