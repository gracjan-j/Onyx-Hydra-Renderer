#pragma once

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rendererPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdOnyxRendererPlugin final : public HdRendererPlugin
{
public:
    HdOnyxRendererPlugin() = default;
    virtual ~HdOnyxRendererPlugin() = default;

    
    virtual HdRenderDelegate *CreateRenderDelegate() override;
    virtual HdRenderDelegate *CreateRenderDelegate(HdRenderSettingsMap const& settingsMap) override;

    
    virtual void DeleteRenderDelegate(HdRenderDelegate *renderDelegate) override;

    
    virtual bool IsSupported(bool gpuEnabled = true) const override;

    
private:
    HdOnyxRendererPlugin(const HdOnyxRendererPlugin&) = delete;
    HdOnyxRendererPlugin &operator =(const HdOnyxRendererPlugin&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE
