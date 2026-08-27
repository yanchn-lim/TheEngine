#pragma once

#include <memory>
#include <string>
#include <vector>

#include "mesh_import_data.hpp"

namespace Ludus::Assets
{
    // isolates file-format parsing from the asset manager
    class IModelImporter
    {
    public:
        virtual ~IModelImporter() = default;
        virtual bool CanImport(const std::string& path) const = 0;
        virtual bool Import(const std::string& path, MeshImportData& outMesh) = 0;
    };

    // selects the first registered importer that accepts a source path
    class ModelImporterRegistry
    {
    public:
        void RegisterImporter(std::unique_ptr<IModelImporter> importer);
        bool Import(const std::string& path, MeshImportData& outMesh) const;
    private:
        std::vector<std::unique_ptr<IModelImporter>> _importers;
    };
}
