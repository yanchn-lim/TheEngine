#include "model_data.hpp"


namespace Assets
{
    bool ModelMeshData::HasStream(VertexSemantic semantic) const
    {
        for (const auto& stream : streams)
        {
            if (stream.semantic == semantic)
                return true;
        }

        return false;
    }

    const VertexStream* ModelMeshData::FindStream(VertexSemantic semantic) const
    {
        for (const auto& stream : streams)
        {
            if (stream.semantic == semantic)
                return &stream;
        }
        return nullptr;
    }

    VertexStream* ModelMeshData::FindStream(VertexSemantic semantic)
    {
        for (auto& stream : streams)
        {
            if (stream.semantic == semantic)
                return &stream;
        }

        return nullptr;
    }
}
