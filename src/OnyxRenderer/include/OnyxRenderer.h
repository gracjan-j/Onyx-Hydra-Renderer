#pragma once

#include <memory>
#include <embree4/rtcore.h>

#include <pxr/base/gf/matrix4d.h>


class OnyxRenderer
{
public:

    struct RenderArgument
    {
        unsigned int width;
        unsigned int height;

        size_t bufferElementSize;
        uint8_t* bufferData;
    };

    OnyxRenderer();
    ~OnyxRenderer();

    bool RenderColorAOV(const RenderArgument& renderArgument);

    void SetCameraMatrices(pxr::GfMatrix4d projMatrix, pxr::GfMatrix4d viewMatrix);

private:

    float RenderEmbreeScene(RTCRayHit ray);

    RTCDevice m_EmbreeDevice;
    RTCScene m_EmbreeScene;
    RTCGeometry m_SceneGeometrySource;

    pxr::GfMatrix4d m_ProjectionMat;
    pxr::GfMatrix4d m_ViewMat;

    pxr::GfMatrix4d m_ProjectionMatInverse;
    pxr::GfMatrix4d m_ViewMatInverse;
};