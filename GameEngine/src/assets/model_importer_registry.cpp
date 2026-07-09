#include "model_importer_registry.hpp"

namespace Assets
{
	void ModelImporterRegistry::RegisterImporter(std::unique_ptr<IModelImporter> importer)
	{
		// Importers are tried in registration order when loading model files.
		if (!importer)
			return;

		_importers.push_back(std::move(importer));
	}

	bool ModelImporterRegistry::Import(const std::string& path, ModelImportData& outModel) const
	{
		// Find the first importer that claims support for this file.
		for (const auto& importer : _importers)
		{
			if (importer->CanImport(path))
				return importer->Import(path, outModel);
		}

		return false;
	}
}
