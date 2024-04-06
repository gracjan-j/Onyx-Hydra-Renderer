#include "renderPass.h"

#include <pxr/imaging/hd/renderPassState.h>
#include <pxr/imaging/hd/renderBuffer.h>

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


HdOnyxRenderPass::HdOnyxRenderPass(
    HdRenderIndex *index
    , HdRprimCollection const &collection
    , const std::shared_ptr<Onyx::OnyxRenderer>& rendererBackend
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

    UnmapAllBuffersFromArgument();
    m_AovBindingVector->clear();

    m_RenderArgument.reset();
    m_RenderThread.reset();
    m_RendererBackend.reset();

    std::cout << "[hdOnyx] Destrukcja Render Pass" << std::endl;
}


void HdOnyxRenderPass::Initialise(HdRenderPassStateSharedPtr const& renderPassState)
{
    // Pobieramy wektor buforów AOV
    m_AovBindingVector = renderPassState->GetAovBindings();

    if (!m_RenderArgument)
    {
        m_RenderArgument = std::make_shared<Onyx::RenderArgument>();
        m_RendererBackend->SetRenderArgument(m_RenderArgument);
    }

    // Mapujemy wszystkie bufory AOV do struktury przekazywanej silnikowi..
    MapAllBuffersToArgument();

    m_RenderArgument->Width = m_AovBindingVector.value()[0].renderBuffer->GetWidth();
    m_RenderArgument->Height = m_AovBindingVector.value()[0].renderBuffer->GetHeight();

    CheckAndUpdateArgumentMatrices(renderPassState);
}


void HdOnyxRenderPass::UnmapAllBuffersFromArgument()
{
    // Odmapowanie buforów wyjściowych oznacza brak wyjścia dla danych silnika.
    // Zatrzymujemy operację renderowania jeśli jest aktualnym stanem silnika.
    if (m_RenderThread->IsRendering()) m_RenderThread->StopRender();

    // Wykonujemy operację unmap na każdym buforze
    for (auto& aovBinding: m_AovBindingVector.value())
    {
        auto* aovBuffer = static_cast<HdOnyxRenderBuffer*>(aovBinding.renderBuffer);

        if (aovBuffer->IsMapped())
        {
            aovBuffer->Unmap();
        }
    }

    m_RenderArgument->MappedBuffers.clear();
    m_RenderArgument->MappedLayout.clear();
}


void HdOnyxRenderPass::MapAllBuffersToArgument()
{
    // Zatrzymujemy operację renderowania jeśli jest aktualnym stanem silnika.
    if (m_RenderThread->IsRendering()) m_RenderThread->StopRender();

    // Mapujemy wszystkie wiązania AOV do mapy buforów
    for (auto& aovBinding: m_AovBindingVector.value())
    {
        auto* aovBuffer = static_cast<HdOnyxRenderBuffer*>(aovBinding.renderBuffer);

        // Mapujemy bufor (stajemy się jego użytkownikami)
        auto mapBuffer = std::pair<void*, size_t>(aovBuffer->Map(), HdDataSizeOfFormat(aovBuffer->GetFormat()));

        // Dodajemy wskaźnik do mapy wskaźników
        m_RenderArgument->MappedBuffers.emplace_back(mapBuffer);

        auto mapLayout = std::pair<pxr::TfToken, uint>(aovBinding.aovName, m_RenderArgument->MappedBuffers.size() - 1);

        // Dodajemy informacje o zmapowanym buforze oraz jego indeksie w wektorze buforów
        m_RenderArgument->MappedLayout.emplace_back(mapLayout);
    }
}


void HdOnyxRenderPass::CheckAndUpdateArgumentBindings(HdRenderPassStateSharedPtr const& renderPassState)
{
    auto& newFrameBindings = renderPassState->GetAovBindings();

    // Jeśli nowy stan określa taką samą ilość AOV
    if (m_AovBindingVector->size() == newFrameBindings.size())
    {
        auto* renderBuffer = static_cast<HdOnyxRenderBuffer*>(newFrameBindings[0].renderBuffer);

        // Jeśli nie zmieniły się wymiary buforów
        // (wszystkie bufory mają takie same wymiary, sprawdzamy pierwszy)
        if (!m_RenderArgument->SizeChanged(renderBuffer->GetWidth(), renderBuffer->GetHeight()))
        {
            // Wychodzimy z metody.
            return;
        }
    }

    // Czeka nas aktualizacja danych. Zatrzymujemy silnik.
    // Przestajemy być użytkownikami render-buforów.
    UnmapAllBuffersFromArgument();

    // Dokonujemy inicjalizacji na nowo, bufory zostaną zmapowane.
    Initialise(renderPassState);
}


void HdOnyxRenderPass::CheckAndUpdateArgumentMatrices(HdRenderPassStateSharedPtr const& renderPassState)
{
    auto passProjection = renderPassState->GetProjectionMatrix();
    auto passView = renderPassState->GetWorldToViewMatrix();

    // Sprawdzamy zmiany w danych macierzy
    if (!m_RenderArgument->ProjectionChanged(passProjection, passView)) return;

    // Zmiana macierzy nie wymaga zmian w mapowaniu buforów. Podmieniamy jedynie dane.
    m_RenderArgument->MatrixInverseProjection = passProjection.GetInverse();
    m_RenderArgument->MatrixInverseView = passView.GetInverse();
}


void HdOnyxRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags)
{
    std::cout << "[hdOnyx] => Wykonanie RenderPass" << std::endl;

    if (renderPassState->GetAovBindings().empty())
    {
        std::cout << "[!][hdOnyx] Brak AOV." << std::endl;
        return;
    }

    if (!m_AovBindingVector.has_value())
    {
        Initialise(renderPassState);
    }
    else
    {
        // Sprawdzamy czy dane dotyczące buforów AOV oraz kamery są aktualne.
        CheckAndUpdateArgumentBindings(renderPassState);
        CheckAndUpdateArgumentMatrices(renderPassState);
    }

    if(!m_RenderThread->IsRendering()) m_RenderThread->StartRender();
}


bool HdOnyxRenderPass::IsConverged() const
{
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
