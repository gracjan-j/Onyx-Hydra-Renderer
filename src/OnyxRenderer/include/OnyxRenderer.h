#pragma once

#include <memory>
#include <embree4/rtcore.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/renderThread.h>


class OnyxRenderer
{
public:

    struct RenderArgument
    {
        unsigned int width;
        unsigned int height;

        using BufferDataPair = std::pair<void*, size_t>;
        using BufferLayoutPair = std::pair<pxr::TfToken, uint>;

        // Dane bufora, wielkość elementu bufora
        std::vector<BufferDataPair> mappedBuffers;

        // Identyfikator bufora, index w mappedBuffers.
        std::vector<BufferLayoutPair> mappedLayout;

        std::optional<BufferDataPair> GetBufferData(const pxr::TfToken& AovToken) const
        {
            // Bufor dla wymaganego AOV jest niedostępny
            uint foundBufferIndex;
            if(!IsAvailable(AovToken, foundBufferIndex)) return std::nullopt;

            return { mappedBuffers[foundBufferIndex] };
        }

        bool SizeChanged(uint testWidth, uint testHeight) const
        {
            if (testHeight != height || (testWidth != width)) return true;
            return false;
        }

        bool IsAvailable(pxr::TfToken AovToken, uint& indexInMap) const
        {
            // Szukamy bufora w mapie buforów za pomocą nazwy AOV
            for (auto& mapPair : mappedLayout)
            {
                if (mapPair.first != AovToken) continue;

                // Token odpowiada szukanemu, upewniamy się że identyfikator jest poprawny
                if (mapPair.second >= mappedBuffers.size()) return false;

                // Index zdaje się poprawny, sprawdzamy czy wskaźnik jest poprawny.
                if (mappedBuffers[mapPair.second].first)
                {
                    // Wskaźnik jest poprawny.
                    indexInMap = mapPair.second;
                    return true;
                }
            }

            return false;
        }
    };

    OnyxRenderer();
    ~OnyxRenderer();

    void MainRenderingEntrypoint(pxr::HdRenderThread* renderThread);

    bool RenderAllAOV();

    void SetCameraMatrices(pxr::GfMatrix4d projMatrix, pxr::GfMatrix4d viewMatrix);
    void SetRenderArguments(const RenderArgument& renderArgument) { m_RenderArgument = renderArgument; }

    uint AttachGeometryToScene(const RTCGeometry& geometrySource);

    void DetachGeometryFromScene(uint geometryID);

    RTCDevice GetEmbreeDeviceHandle() const { return m_EmbreeDevice; };
    RTCScene GetEmbreeSceneHandle() const { return m_EmbreeScene; };

private:

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
