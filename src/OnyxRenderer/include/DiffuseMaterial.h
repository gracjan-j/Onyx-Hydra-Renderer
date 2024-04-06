#pragma once

#include "Material.h"


namespace Onyx
{

    class DiffuseMaterial: public Material
    {
    public:

        DiffuseMaterial() = delete;

        DiffuseMaterial(const pxr::GfVec3f& diffuseReflectance)
        : m_DiffuseReflectance{diffuseReflectance}
        {}

        pxr::GfVec3f Sample() override;

        pxr::GfVec3f Evaluate() override;

        float PDF(pxr::GfVec3f sample) override;

    private:
        pxr::GfVec3f m_DiffuseReflectance;
    };

}


