#pragma once

#include <memory>
#include <string>
#include <vector>

#include "mesh_source_data.hpp"

namespace Assets
{
    class IModelImporter
    {
    public:
        virtual ~IModelImporter() = default;
        virtual bool CanLoad(const std::string& path) const = 0;
        virtual bool Load(const std::string& path, MeshSourceCollection& outModel) = 0;
    };

    class ModelLoader
    {
    public:
        void RegisterImporter(std::unique_ptr<IModelImporter> importer);
        bool Load(const std::string& path, MeshSourceCollection& outModel) const;
    private:
        std::vector<std::unique_ptr<IModelImporter>> _importers;
    };
}
