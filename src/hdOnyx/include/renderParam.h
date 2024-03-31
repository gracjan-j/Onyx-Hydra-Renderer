#pragma once

#include "OnyxRenderer.h"

#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/renderThread.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdOnyxRenderParam final : public HdRenderParam
{
public:

    HdOnyxRenderParam(RTCDevice embreeDevice, OnyxRenderer* rendererBackend)
    : m_EmbreeDevice{embreeDevice}
    , m_RendererBackend(rendererBackend)
    {}

    RTCDevice GetEmbreeDevice() { return m_EmbreeDevice; }
    OnyxRenderer* GetRendererHandle() { return m_RendererBackend; }

private:

    RTCDevice m_EmbreeDevice;
    OnyxRenderer* m_RendererBackend;
    std::shared_ptr<HdRenderThread> m_BackgroundRenderThread;
};

PXR_NAMESPACE_CLOSE_SCOPE
