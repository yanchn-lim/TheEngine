#pragma once

#include <memory>
#include <string>
#include <vector>

#include "mesh_import_data.hpp"

namespace Assets
{
    class IModelImporter
    {
    public:
        virtual ~IModelImporter() = default;
        virtual bool CanImport(const std::string& path) const = 0;
        virtual bool Import(const std::string& path, ModelImportData& outModel) = 0;
    };

    class ModelImporterRegistry
    {
    public:
        void RegisterImporter(std::unique_ptr<IModelImporter> importer);
        bool Import(const std::string& path, ModelImportData& outModel) const;
    private:
        std::vector<std::unique_ptr<IModelImporter>> _importers;
    };
}
