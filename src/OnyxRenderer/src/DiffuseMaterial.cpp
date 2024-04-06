#include "DiffuseMaterial.h"


pxr::GfVec3f Onyx::DiffuseMaterial::Sample()
{
    return {0.0, 0.0, 1.0};
}

pxr::GfVec3f Onyx::DiffuseMaterial::Evaluate()
{
    return m_DiffuseReflectance;
}

float Onyx::DiffuseMaterial::PDF(pxr::GfVec3f sample)
{
    return 1.0;
}
