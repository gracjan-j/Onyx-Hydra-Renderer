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
        , const std::shared_ptr<OnyxRenderer>& rendererBackend
        , HdRenderThread* backgroundRenderThread);

    virtual ~HdOnyxRenderPass();


protected:
    void _Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags) override;

    bool IsConverged() const override;


private:
    void CheckAndUpdateRendererData(HdRenderPassStateSharedPtr const& renderPassState, HdOnyxRenderBuffer& renderBuffer);

    void RunRenderBackendForColorAOV(HdRenderPassStateSharedPtr const& renderPassState, HdOnyxRenderBuffer& colorAOVBuffer);
    void RunRenderDebugForNormalAOV(HdRenderPassStateSharedPtr const& renderPassState, HdOnyxRenderBuffer& colorAOVBuffer);

    std::shared_ptr<OnyxRenderer> m_RendererBackend;

    HdRenderThread* m_RenderThread;

    // TODO: [Prototyp] Spraw żeby update danych RenderArgument były możliwe.
    bool updateOnce = true;
};


PXR_NAMESPACE_CLOSE_SCOPE
