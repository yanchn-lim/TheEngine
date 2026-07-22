#pragma once

#include "model_importer_registry.hpp"

namespace Assets
{
    // converts OBJ files into the engine's backend-neutral import structures
    class ObjImporter : public IModelImporter
    {
    public:
        bool CanImport(const std::string& path) const override;
        bool Import(const std::string& path, ModelImportData& outModel) override;
    };
}
