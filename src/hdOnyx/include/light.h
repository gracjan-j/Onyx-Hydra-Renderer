#pragma once

#include <pxr/imaging/hd/light.h>
#include <pxr/base/gf/matrix4d.h>

#include "mesh.h"


PXR_NAMESPACE_OPEN_SCOPE


class HdOnyxLight final : public HdLight
{
public:

    HdOnyxLight(SdfPath const& id);
    ~HdOnyxLight();

    void Sync(HdSceneDelegate* sceneDelegate,
              HdRenderParam* renderParam,
              HdDirtyBits* dirtyBits) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

private:

    /**
     * Transformacja instancji światła w scenie.
     * Uwzględnia skalowanie wynikające z parametrów rozmiaru (width & height)
     * w efekcie na rozmiar światła wpływa skalowanie macierzy oraz jego parametry.
     */
    GfMatrix4d m_InstanceTransformation;

    /**
     * Macierz skalująca wykonana na podstawie parametrów rozmiaru światła.
     * W przypadku RectLight są to width i height.
     */
    GfMatrix4d m_ParameterScalingTransformation;

    /**
     * Całkowity rozmiar światła w osi X w lokalnym układzie współrzędnych.
     */
    float m_Width = 1.0;

    /**
     * Całkowity rozmiar światła w osi Y w lokalnym układzie współrzędnych.
     */
    float m_Height = 1.0;

    /**
     * Siła emisji światła. Dokumentacja świateł OpenUSD zakłada:
     * Emissive Power = (intensity * 2^exposure)
     *
     * Siła emisji jest mnożona przez kolor emisji (RGB).
     */
    GfVec3f m_TotalEmissivePower;

    HdOnyxInstanceData m_LightInstanceData;
};


PXR_NAMESPACE_CLOSE_SCOPE