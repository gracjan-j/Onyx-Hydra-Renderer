#pragma once

#include "OnyxRenderer.h"
#include "renderParam.h"
#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include <pxr/imaging/hd/renderThread.h>

PXR_NAMESPACE_OPEN_SCOPE


class HdOnyxRenderDelegate final : public HdRenderDelegate
{
public:
    HdOnyxRenderDelegate();
    HdOnyxRenderDelegate(HdRenderSettingsMap const& settingsMap);

    virtual ~HdOnyxRenderDelegate();

    const TfTokenVector &GetSupportedRprimTypes() const override;
    const TfTokenVector &GetSupportedSprimTypes() const override;
    const TfTokenVector &GetSupportedBprimTypes() const override;

    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    HdRenderPassSharedPtr CreateRenderPass(
        HdRenderIndex *index,
        HdRprimCollection const& collection) override;

    HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                         SdfPath const& id) override;
    void DestroyInstancer(HdInstancer *instancer) override;

    HdRprim *CreateRprim(TfToken const& typeId, SdfPath const& rprimId) override;
    HdSprim *CreateSprim(TfToken const& typeId, SdfPath const& sprimId) override;
    HdBprim *CreateBprim(TfToken const& typeId, SdfPath const& bprimId) override;

    void DestroyRprim(HdRprim *rPrim) override;
    void DestroySprim(HdSprim *sprim) override;
    void DestroyBprim(HdBprim *bprim) override;

    HdSprim *CreateFallbackSprim(TfToken const& typeId) override;
    HdBprim *CreateFallbackBprim(TfToken const& typeId) override;

    void CommitResources(HdChangeTracker *tracker) override;

    HdRenderParam *GetRenderParam() const override;

    HdAovDescriptor GetDefaultAovDescriptor(const TfToken& aovName) const override;

    bool IsPauseSupported() const override;
    bool Pause() override;
    bool Resume() override;

private:

    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    HdResourceRegistrySharedPtr m_ResourceRegistry;
    std::shared_ptr<Onyx::OnyxRenderer> m_RendererBackend;

    // Używamy funkcjonalności USD która pozwala na utworzenie
    // osobnego wątku do renderowania. RenderThread jest niewielką
    // nakładką na std::thread która wspiera popularne komendy typu
    // Stop, Pauza, Render.
    // Główną motywacją do użycia tej funkcjonalnosci jest
    // wolne działanie interfejsu użytkownika w trybie jednowątkowym.
    std::shared_ptr<HdRenderThread> m_BackgroundRenderThread;

    // Struktura służąca do wymiany informacji podczas synchronizacji primów.
    // Przekazuje obiekty służące do generacji reprezentacji geometrii Embree
    // oraz wskaźnik do backendu silnika w celu wywoływania modyfikacji danych.
    std::shared_ptr<HdOnyxRenderParam> m_RenderParam;

    void _Initialize();
    void _RenderCallback();

    HdOnyxRenderDelegate(const HdOnyxRenderDelegate &) = delete;
    HdOnyxRenderDelegate &operator =(const HdOnyxRenderDelegate &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE
