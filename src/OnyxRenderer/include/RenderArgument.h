#pragma once

#include <sys/types.h>

namespace Onyx
{
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

}