#include "renderPass.h"

#include <pxr/imaging/hd/renderPassState.h>
#include <pxr/imaging/hd/renderBuffer.h>

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


HdOnyxRenderPass::HdOnyxRenderPass(
    HdRenderIndex *index
    , HdRprimCollection const &collection
    , const std::shared_ptr<OnyxRenderer>& rendererBackend
    , HdRenderThread* backgroundRenderThread)
    : HdRenderPass(index, collection)
{
    // Współdzielony wskaźnik
    m_RendererBackend = rendererBackend;

    m_RenderThread = backgroundRenderThread;
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


void HdOnyxRenderPass::RunRenderDebugForNormalAOV(
    HdRenderPassStateSharedPtr const& renderPassState,
    HdOnyxRenderBuffer& normalAOVBuffer
){
    // W momencie wywołania metody Map() stajemy się
    // użytkownikami bufora, dostajemy wskaźnik do danych.
    auto* bufferData = static_cast<uint8_t*>(normalAOVBuffer.Map());

    OnyxRenderer::RenderArgument argument = {
        .width = normalAOVBuffer.GetWidth(),
        .height = normalAOVBuffer.GetHeight(),
        .bufferElementSize = HdDataSizeOfFormat(normalAOVBuffer.GetFormat()),
        .bufferData = bufferData
    };

    // Pobieramy macierze kamery. RenderPass otrzymuje macierze od silnika UsdImagingGLEngine.
    // Niezbędne jest ustawienie poprawnej ścieżki kamery przed wywołaniem renderowania.
    const GfMatrix4d viewMatrix = renderPassState->GetWorldToViewMatrix();
    const GfMatrix4d projMatrix = renderPassState->GetProjectionMatrix();

    m_RendererBackend->SetCameraMatrices(projMatrix, viewMatrix);
    // m_RendererBackend->RenderDebugNormalAOV(argument);

    // Po wykonaniu renderu możemy zwolnić bufor z użycia.
    normalAOVBuffer.Unmap();
}


void HdOnyxRenderPass::CheckAndUpdateRendererData(HdRenderPassStateSharedPtr const& renderPassState, HdOnyxRenderBuffer& renderBuffer)
{
    // W momencie wywołania metody Map() stajemy się
    // użytkownikami bufora, dostajemy wskaźnik do danych.
    auto* bufferData = static_cast<uint8_t*>(renderBuffer.Map());

    OnyxRenderer::RenderArgument argument = {
        .width = renderBuffer.GetWidth(),
        .height = renderBuffer.GetHeight(),
        .bufferElementSize = HdDataSizeOfFormat(renderBuffer.GetFormat()),
        .bufferData = bufferData
    };

    // Pobieramy macierze kamery. RenderPass otrzymuje macierze od silnika UsdImagingGLEngine.
    // Niezbędne jest ustawienie poprawnej ścieżki kamery przed wywołaniem renderowania.
    const GfMatrix4d viewMatrix = renderPassState->GetWorldToViewMatrix();
    const GfMatrix4d projMatrix = renderPassState->GetProjectionMatrix();

    m_RenderThread->StopRender();


    m_RendererBackend->SetCameraMatrices(projMatrix, viewMatrix);
    m_RendererBackend->SetRenderArguments(argument);

    m_RenderThread->StartRender();
}


void HdOnyxRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags)
{
    std::cout << "[hdOnyx] => Wykonanie RenderPass" << std::endl;

    HdRenderPassAovBindingVector aovBindingVector = renderPassState->GetAovBindings();

    for (auto& aovBinding : aovBindingVector)
    {
        if (aovBinding.aovName == HdAovTokens->color)
        {
            auto* renderBuffer = static_cast<HdOnyxRenderBuffer*>(aovBinding.renderBuffer);
            if (updateOnce) CheckAndUpdateRendererData(renderPassState, *renderBuffer);
        }

        else if (aovBinding.aovName == HdAovTokens->normal)
        {
            auto* renderBuffer = static_cast<HdOnyxRenderBuffer*>(aovBinding.renderBuffer);
            // RunRenderDebugForNormalAOV(renderPassState, *renderBuffer);
        }


    }

    m_RenderThread->StartRender();
    updateOnce = false;
}

bool HdOnyxRenderPass::IsConverged() const
{
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
