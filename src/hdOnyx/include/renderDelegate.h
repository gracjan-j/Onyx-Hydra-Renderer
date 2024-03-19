#pragma once

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/resourceRegistry.h"

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

    HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId) override;
    void DestroyRprim(HdRprim *rPrim) override;

    HdSprim *CreateSprim(TfToken const& typeId,
                         SdfPath const& sprimId) override;
    HdSprim *CreateFallbackSprim(TfToken const& typeId) override;
    void DestroySprim(HdSprim *sprim) override;

    HdBprim *CreateBprim(TfToken const& typeId,
                                 SdfPath const& bprimId) override;
    HdBprim *CreateFallbackBprim(TfToken const& typeId) override;
    void DestroyBprim(HdBprim *bprim) override;

    void CommitResources(HdChangeTracker *tracker) override;

    HdRenderParam *GetRenderParam() const override;

private:
    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    void _Initialize();

    HdResourceRegistrySharedPtr m_ResourceRegistry;

    HdOnyxRenderDelegate(const HdOnyxRenderDelegate &) = delete;
    HdOnyxRenderDelegate &operator =(const HdOnyxRenderDelegate &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE
