#pragma once

#include <sys/types.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/gf/matrix4d.h>

namespace Onyx
{
    struct RenderArgument
    {
        unsigned int Width;
        unsigned int Height;

        pxr::GfMatrix4d MatrixInverseProjection;
        pxr::GfMatrix4d MatrixInverseView;

        using BufferDataPair = std::pair<void*, size_t>;
        using BufferLayoutPair = std::pair<pxr::TfToken, uint>;

        // Dane bufora, wielkość elementu bufora
        std::vector<BufferDataPair> MappedBuffers;

        // Identyfikator bufora, index w mappedBuffers.
        std::vector<BufferLayoutPair> MappedLayout;

        std::optional<BufferDataPair> GetBufferData(const pxr::TfToken& AovToken) const
        {
            // Bufor dla wymaganego AOV jest niedostępny
            uint foundBufferIndex;
            if(!IsAvailable(AovToken, foundBufferIndex)) return std::nullopt;

            return { MappedBuffers[foundBufferIndex] };
        }


        bool SizeChanged(uint testWidth, uint testHeight) const
        {
            if (testHeight != Height || (testWidth != Width)) return true;
            return false;
        }

        bool ProjectionChanged(const pxr::GfMatrix4d& newProjection, const pxr::GfMatrix4d& newView)
        {
            if(MatrixInverseProjection != newProjection || (MatrixInverseView != newView)) return true;
            return false;
        }

        bool IsAvailable(pxr::TfToken AovToken, uint& indexInMap) const
        {
            // Szukamy bufora w mapie buforów za pomocą nazwy AOV
            for (auto& mapPair : MappedLayout)
            {
                if (mapPair.first != AovToken) continue;

                // Token odpowiada szukanemu, upewniamy się że identyfikator jest poprawny
                if (mapPair.second >= MappedBuffers.size()) return false;

                // Index zdaje się poprawny, sprawdzamy czy wskaźnik jest poprawny.
                if (MappedBuffers[mapPair.second].first)
                {
                    // Wskaźnik jest poprawny.
                    indexInMap = mapPair.second;
                    return true;
                }
            }

            return false;
        }
    };

}