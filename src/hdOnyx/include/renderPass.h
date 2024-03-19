#pragma once

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderPass.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdOnyxRenderPass final : public HdRenderPass
{
public:

    HdOnyxRenderPass(HdRenderIndex *index, HdRprimCollection const &collection);

    virtual ~HdOnyxRenderPass();

protected:

    void _Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags) override;
};


PXR_NAMESPACE_CLOSE_SCOPE
