#pragma once

#include "renderBuffer.h"

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderPass.h"

#include <OnyxRenderer.h>


PXR_NAMESPACE_OPEN_SCOPE


class HdOnyxRenderPass final : public HdRenderPass
{
public:
    HdOnyxRenderPass(
        HdRenderIndex *index
        , HdRprimCollection const &collection
        , const std::shared_ptr<Onyx::OnyxRenderer>& rendererBackend
        , const std::shared_ptr<HdRenderThread>& backgroundRenderThread);

    virtual ~HdOnyxRenderPass();

protected:
    void _Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags) override;

    bool IsConverged() const override;


private:
    void Initialise(HdRenderPassStateSharedPtr const& renderPassState);

    void MapAllBuffersToArgument();
    void UnmapAllBuffersFromArgument();


    void CheckAndUpdateArgumentBindings(HdRenderPassStateSharedPtr const& renderPassState);
    void CheckAndUpdateArgumentMatrices(HdRenderPassStateSharedPtr const& renderPassState);


    // Współdzielony wskaźnik do backendu silnika renderującego.
    // Używany do przekazywania danych projekcji oraz buforów pikseli które wymagają wypełnienia.
    std::shared_ptr<Onyx::OnyxRenderer> m_RendererBackend;

    // Współdzielony wskaźnik do wątku który wykonuje renderowanie.
    // Używany do kontroli wątku (Stop, Start) w przypadku aktualizacji danych.
    std::shared_ptr<HdRenderThread> m_RenderThread;

    // Wektor renderowanych AOV (arbitrary output variables).
    // AOV są buforami danych wyświetlanych a ekranie / wymaganych przez
    // zewnętrzną logikę (głębia, wektory normalne).
    std::optional<HdRenderPassAovBindingVector> m_AovBindingVector;

    // Flaga wskazująca na poprawność danych buforów wyjściowych
    // przekazanych silnikowi poprzez strukturę RenderArgument silnika.
    bool m_ArgumentSendRequired = true;

    std::shared_ptr<Onyx::RenderArgument> m_RenderArgument;
};


PXR_NAMESPACE_CLOSE_SCOPE
