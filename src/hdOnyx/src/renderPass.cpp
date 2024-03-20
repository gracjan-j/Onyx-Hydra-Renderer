#include "renderPass.h"

#include <pxr/imaging/hd/renderPassState.h>
#include <pxr/imaging/hd/renderBuffer.h>

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


HdOnyxRenderPass::HdOnyxRenderPass(HdRenderIndex *index, HdRprimCollection const &collection)
: HdRenderPass(index, collection)
{

}


HdOnyxRenderPass::~HdOnyxRenderPass()
{
    std::cout << "[hdOnyx] Destrukcja Render Pass" << std::endl;
}


void DrawColor(HdRenderBuffer* colorBuffer)
{
    // Pobieramy informacje o formacie danych bufora.
    HdFormat format = colorBuffer->GetFormat();

    // Obliczamy rozmiar elementu z formatu OpenUSD.
    size_t formatSize = HdDataSizeOfFormat(format);

    // Stajemy się użytkownikami bufora, dostajemy wskaźnik do danych.
    uint8_t* bufferData = static_cast<uint8_t*>(colorBuffer->Map());

    // Dla każdego pixela w buforze.
    for (auto currentY = 0; currentY < colorBuffer->GetHeight(); currentY++)
    {
        for (auto currentX = 0; currentX < colorBuffer->GetWidth(); currentX++)
        {
            // Obliczamy jedno-wymiarowy offset pixela w buforze 2D.
            uint32_t pixelOffsetInBuffer = (currentY * colorBuffer->GetWidth()) + currentX;

            // Compute the byte offset, based on raster format.
            uint8_t* pixelData = &bufferData[pixelOffsetInBuffer * formatSize];

            // Wpisujemy wartość piksela na podstawie pozycji punktu
            // tworząc gradient.
            pixelData[0] = int((float(currentX) / colorBuffer->GetWidth()) * 255);
            pixelData[1] = int((float(currentY) / colorBuffer->GetHeight()) * 255);
            pixelData[2] = 255;
            pixelData[3] = 255;
        }
    }

    // End buffer write.
    colorBuffer->Unmap();
}


void HdOnyxRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags)
{
    std::cout << "[hdOnyx] => Wykonanie RenderPass" << std::endl;

    // Iterate over each AOV.
    HdRenderPassAovBindingVector aovBindingVector = renderPassState->GetAovBindings();

    for (auto& aovBinding : aovBindingVector) {

        if (aovBinding.aovName == HdAovTokens->color) {
            HdRenderBuffer* renderBuffer = aovBinding.renderBuffer;
            DrawColor(renderBuffer);
        }
        //
        // if (aovBinding.aovName == HdAovTokens->normal) {
        //     HdTriRenderBuffer* renderBuffer =
        //         static_cast<HdTriRenderBuffer*>(aovBinding.renderBuffer);
        //     HdTriRenderer::DrawTriangle(renderBuffer);
        // }
    }
}

bool HdOnyxRenderPass::IsConverged() const
{
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
