#include "renderPass.h"

#include <pxr/imaging/hd/renderPassState.h>
#include <pxr/imaging/hd/renderBuffer.h>

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


HdOnyxRenderPass::HdOnyxRenderPass(
    HdRenderIndex *index
    , HdRprimCollection const &collection
    , const std::shared_ptr<OnyxRenderer>& rendererBackend)
    : HdRenderPass(index, collection)
{
    // Współdzielony wskaźnik
    m_RendererBackend = rendererBackend;
}


HdOnyxRenderPass::~HdOnyxRenderPass()
{
    std::cout << "[hdOnyx] Destrukcja Render Pass" << std::endl;
}


void HdOnyxRenderPass::RunRenderBackendForColorAOV(HdRenderPassStateSharedPtr const& renderPassState, HdOnyxRenderBuffer& colorAOVBuffer)
{
    // W momencie wywołania metody Map() stajemy się
    // użytkownikami bufora, dostajemy wskaźnik do danych.
    auto* bufferData = static_cast<uint8_t*>(colorAOVBuffer.Map());

    OnyxRenderer::RenderArgument argument = {
        .width = colorAOVBuffer.GetWidth(),
        .height = colorAOVBuffer.GetHeight(),
        .bufferElementSize = HdDataSizeOfFormat(colorAOVBuffer.GetFormat()),
        .bufferData = bufferData
    };

    // Pobieramy macierze kamery. RenderPass otrzymuje macierze od silnika UsdImagingGLEngine.
    // Niezbędne jest ustawienie poprawnej ścieżki kamery przed wywołaniem renderowania.
    const GfMatrix4d viewMatrix = renderPassState->GetWorldToViewMatrix();
    const GfMatrix4d projMatrix = renderPassState->GetProjectionMatrix();

    m_RendererBackend->SetCameraMatrices(projMatrix, viewMatrix);
    m_RendererBackend->RenderColorAOV(argument);

    // Po wykonaniu renderu możemy zwolnić bufor z użycia.
    colorAOVBuffer.Unmap();
}


void HdOnyxRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags)
{
    std::cout << "[hdOnyx] => Wykonanie RenderPass" << std::endl;

    // Iterate over each AOV.
    HdRenderPassAovBindingVector aovBindingVector = renderPassState->GetAovBindings();

    for (auto& aovBinding : aovBindingVector)
    {
        if (aovBinding.aovName == HdAovTokens->color)
        {
            auto* renderBuffer = static_cast<HdOnyxRenderBuffer*>(aovBinding.renderBuffer);
            RunRenderBackendForColorAOV(renderPassState, *renderBuffer);
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
