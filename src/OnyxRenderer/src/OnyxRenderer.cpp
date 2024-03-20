#include "OnyxRenderer.h"


bool OnyxRenderer::RenderColorAOV(const RenderArgument& renderArgument)
{
    // Dla każdego pixela w buforze.
    for (auto currentY = 0; currentY < renderArgument.height; currentY++)
    {
        for (auto currentX = 0; currentX < renderArgument.width; currentX++)
        {
            // Obliczamy jedno-wymiarowy offset pixela w buforze 2D.
            uint32_t pixelOffsetInBuffer = (currentY * renderArgument.width) + currentX;

            uint8_t* bufferContents = renderArgument.bufferData;
            uint8_t* pixelData = &bufferContents[pixelOffsetInBuffer * renderArgument.bufferElementSize];

            // Wpisujemy wartość piksela na podstawie pozycji punktu
            // tworząc gradient.
            pixelData[0] = int((float(currentX) / renderArgument.width) * 255);
            pixelData[1] = int((float(currentY) / renderArgument.height) * 255);
            pixelData[2] = 255;
            pixelData[3] = 255;
        }
    }

    return true;
}