#pragma once

#include <memory>
#include <embree4/rtcore.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/imaging/hd/renderThread.h>


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

    void MainRenderingEntrypoint(pxr::HdRenderThread* renderThread);

    bool RenderColorAOV(const RenderArgument& renderArgument);
    bool RenderDebugNormalAOV();

    void SetCameraMatrices(pxr::GfMatrix4d projMatrix, pxr::GfMatrix4d viewMatrix);
    void SetRenderArguments(const RenderArgument& renderArgument) { m_RenderArgument = renderArgument; }

    uint AttachGeometryToScene(const RTCGeometry& geometrySource);

    void DetachGeometryFromScene(uint geometryID);

    RTCDevice GetEmbreeDeviceHandle() const { return m_EmbreeDevice; };
    RTCScene GetEmbreeSceneHandle() const { return m_EmbreeScene; };

private:

    float RenderEmbreeScene(RTCRayHit ray);

    RTCRayHit GeneratePrimaryRay(float offsetX, float offsetY, const RenderArgument& renderArgument);

    RTCDevice m_EmbreeDevice;
    RTCScene m_EmbreeScene;

    // Macierze perspektywy oraz widoku kamery wraz z inwersjami.
    // TODO: Dedykowana klasa Kamery, przygotowywanie RTCRay do Embree.
    pxr::GfMatrix4d m_ProjectionMat;
    pxr::GfMatrix4d m_ViewMat;
    pxr::GfMatrix4d m_ProjectionMatInverse;
    pxr::GfMatrix4d m_ViewMatInverse;

    RenderArgument m_RenderArgument;
};
