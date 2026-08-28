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

    // owns importers and selects the first one that accepts a source path.
    // an import failure does not fall through to later importers.
    class ModelImporterRegistry
    {
    public:
        void RegisterImporter(std::unique_ptr<IModelImporter> importer);
        bool Import(const std::string& path, MeshImportData& outMesh) const;
    private:
        std::vector<std::unique_ptr<IModelImporter>> _importers;
    };
}
