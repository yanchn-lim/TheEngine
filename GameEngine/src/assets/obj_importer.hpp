#pragma once

#include "model_importer_registry.hpp"

namespace Assets
{
    class ObjImporter : public IModelImporter
    {
    public:
        bool CanImport(const std::string& path) const override;
        bool Import(const std::string& path, ModelImportData& outModel) override;
    };
}
