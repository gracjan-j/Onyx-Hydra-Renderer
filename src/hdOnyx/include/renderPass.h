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
        , const std::shared_ptr<OnyxRenderer>& rendererBackend);

    virtual ~HdOnyxRenderPass();


protected:
    void _Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags) override;

    bool IsConverged() const override;


private:

    void RunRenderBackendForColorAOV(HdRenderPassStateSharedPtr const& renderPassState, HdOnyxRenderBuffer& colorAOVBuffer);

    std::shared_ptr<OnyxRenderer> m_RendererBackend;
};


PXR_NAMESPACE_CLOSE_SCOPE
