#include "renderPass.h"

#include <pxr/imaging/hd/renderPassState.h>
#include <pxr/imaging/hd/renderBuffer.h>

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


HdOnyxRenderPass::HdOnyxRenderPass(
    HdRenderIndex *index
    , HdRprimCollection const &collection
    , const std::shared_ptr<OnyxRenderer>& rendererBackend
    , const std::shared_ptr<HdRenderThread>& backgroundRenderThread)
    : HdRenderPass(index, collection)
{
    m_RendererBackend = rendererBackend;
    m_RenderThread = backgroundRenderThread;
}


HdOnyxRenderPass::~HdOnyxRenderPass()
{
    // Jeśli wątek renderuje, zatrzymujemy
    if (m_RenderThread->IsRendering()) m_RenderThread->StopRender();

    m_RenderThread.reset();
    m_RendererBackend.reset();

    std::cout << "[hdOnyx] Destrukcja Render Pass" << std::endl;
}


void HdOnyxRenderPass::CheckAndUpdateRendererData(HdRenderPassStateSharedPtr const& renderPassState)
{
    // Pobieramy macierze kamery. RenderPass otrzymuje macierze od silnika UsdImagingGLEngine.
    // Niezbędne jest ustawienie poprawnej ścieżki kamery przed wywołaniem renderowania.
    const GfMatrix4d viewMatrix = renderPassState->GetWorldToViewMatrix();
    const GfMatrix4d projMatrix = renderPassState->GetProjectionMatrix();

    if(m_RenderThread->IsRendering()) m_RenderThread->StopRender();

    m_RendererBackend->SetCameraMatrices(projMatrix, viewMatrix);
}


void HdOnyxRenderPass::UnmapAllBuffersFromArgument()
{
    // Odmapowanie buforów wyjściowych oznacza brak wyjścia dla danych silnika.
    // Zatrzymujemy operację renderowania jeśli jest aktualnym stanem silnika.
    if (m_RenderThread->IsRendering()) m_RenderThread->StopRender();

    // Dokonujemy invalidacji RenderArgument
    m_LastSentArgument = std::nullopt;
    m_ArgumentSendRequired = true;

    // Wykonujemy operację unmap na każdym buforze
    for (auto& aovBinding : m_AovBindingVector.value())
    {
        auto* aovBuffer = static_cast<HdOnyxRenderBuffer*>(aovBinding.renderBuffer);

        if (aovBuffer->IsMapped())
        {
            aovBuffer->Unmap();
        }
    }
}


void HdOnyxRenderPass::SendRenderArgumentsToEngine()
{
    OnyxRenderer::RenderArgument argumentToSend = {};
    uint width, height = 0;

    // Mapujemy wszystkie wiązania AOV do mapy buforów
    for (auto& aovBinding : m_AovBindingVector.value())
    {
        auto* aovBuffer = static_cast<HdOnyxRenderBuffer*>(aovBinding.renderBuffer);
        width = aovBuffer->GetWidth(); height = aovBuffer->GetHeight();

        // Mapujemy bufor (stajemy się jego użytkownikami)
        auto mapBuffer = std::pair<void*, size_t>(aovBuffer->Map(), HdDataSizeOfFormat(aovBuffer->GetFormat()));

        // Dodajemy wskaźnik do mapy wskaźników
        argumentToSend.mappedBuffers.emplace_back(mapBuffer);

        auto mapLayout = std::pair<pxr::TfToken, uint>(aovBinding.aovName, argumentToSend.mappedBuffers.size() - 1);

        // Dodajemy informacje o zmapowanym buforze oraz jego indeksie w wektorze buforów
        argumentToSend.mappedLayout.emplace_back(mapLayout);
    }

    argumentToSend.height = height;
    argumentToSend.width = width;

    // Wysyłamy argument do silnika. Silnik będzie wypisywał dane wyjściowe do nowych buforów.
    m_RendererBackend->SetRenderArguments(argumentToSend);

    m_LastSentArgument = argumentToSend;
    m_ArgumentSendRequired = false;
}



void HdOnyxRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags)
{
    std::cout << "[hdOnyx] => Wykonanie RenderPass" << std::endl;

    if (renderPassState->GetAovBindings().empty())
    {
        std::cout << "[!][hdOnyx] Brak AOV." << std::endl;
        return;
    }

    // Wektor powiązań nie istnieje, pierwsze wykonanie.
    if (!m_AovBindingVector.has_value())
    {
        m_AovBindingVector = renderPassState->GetAovBindings();
        m_ArgumentSendRequired = true;

        CheckAndUpdateRendererData(renderPassState);
    }

    // Kolejne wykonanie, dokonamy sprawdzenia ważności danych
    if (m_AovBindingVector.has_value() && m_LastSentArgument.has_value())
    {
        auto& stateAovBindings = renderPassState->GetAovBindings();

        // Jeśli nowy stan określa inną ilość AOV, musimy dokonać aktualizacji
        if (m_AovBindingVector->size() != stateAovBindings.size())
        {
            // Ustawia flagę Argument Send Required.
            UnmapAllBuffersFromArgument();
            m_AovBindingVector = stateAovBindings;
        }
        else
        {
            auto* renderBuffer = static_cast<HdOnyxRenderBuffer*>(stateAovBindings[0].renderBuffer);

            if (m_LastSentArgument->SizeChanged(renderBuffer->GetWidth(), renderBuffer->GetHeight()))
            {
                UnmapAllBuffersFromArgument();
                m_AovBindingVector = stateAovBindings;
            }
        }
    }

    if (m_ArgumentSendRequired)
    {
        SendRenderArgumentsToEngine();
    }

    m_RenderThread->StartRender();
}

bool HdOnyxRenderPass::IsConverged() const
{
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
