#pragma once

#include "model_loader.hpp"

namespace Assets
{
    class ObjModelImporter : public IModelImporter
    {
    public:
        bool CanLoad(const std::string& path) const override;
        bool Load(const std::string& path, MeshSourceCollection& outModel) override;
    };
}