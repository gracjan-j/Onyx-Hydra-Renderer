#include "DiffuseMaterial.h"


pxr::GfVec3f Onyx::DiffuseMaterial::Sample()
{
    return m_DiffuseReflectance;
}

pxr::GfVec3f Onyx::DiffuseMaterial::Evaluate() {}

float Onyx::DiffuseMaterial::PDF(pxr::GfVec3f sample) {}
