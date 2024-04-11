#include "DiffuseMaterial.h"

#include <pxr/base/gf/matrix3f.h>
#include <pxr/base/gf/vec2f.h>

#include "OnyxHelper.h"


pxr::GfVec3f Onyx::DiffuseMaterial::Sample(const pxr::GfVec3f& N, const pxr::GfVec2f& random2D)
{
    pxr::GfMatrix3f toWorldTransform = OnyxHelper::GenerateOrthogonalFrameInZ(N);

    // Generujemy próbkę na dysku.
    float sampleTheta = 2.0f * M_PI * random2D[0];

    // Wykonujemy projekcję punktu na płaszczyźnie dysku
    // na powierzchnię hemisfery.
    float sampleEta = random2D[1];

    // Sqrt skupia projekcję bliżej czubka hemisfery.
    float sqrtEta = sqrtf(sampleEta);

    auto localSpaceSample = pxr::GfVec3f {
        cosf(sampleTheta) * sqrtEta,
        sinf(sampleTheta) * sampleEta,
        sqrtf(1.0f - sampleEta)
    };

    return toWorldTransform * localSpaceSample;
}


pxr::GfVec3f Onyx::DiffuseMaterial::Evaluate(const pxr::GfVec3f& sample)
{
    // Funkcja Lambert BRDF jest stała.
    return m_DiffuseReflectance;
}


float Onyx::DiffuseMaterial::PDF(const pxr::GfVec3f& sample)
{
    return 1.0;
}
