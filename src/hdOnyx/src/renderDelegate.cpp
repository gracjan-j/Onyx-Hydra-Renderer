#include "renderDelegate.h"
#include "renderPass.h"
#include "renderBuffer.h"
#include "mesh.h"

#include <pxr/imaging/hd/renderBuffer.h>
#include <pxr/imaging/hd/camera.h>

#include <OnyxRenderer.h>

#include <iostream>


PXR_NAMESPACE_OPEN_SCOPE
    const TfTokenVector HdOnyxRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
};


const TfTokenVector HdOnyxRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->camera,
};


const TfTokenVector HdOnyxRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
    HdPrimTypeTokens->renderBuffer
};

TfTokenVector const& HdOnyxRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const& HdOnyxRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const& HdOnyxRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}


HdOnyxRenderDelegate::HdOnyxRenderDelegate()
: HdRenderDelegate()
{
    _Initialize();
}


HdOnyxRenderDelegate::HdOnyxRenderDelegate(HdRenderSettingsMap const& settingsMap)
: HdRenderDelegate(settingsMap)
{
    _Initialize();
}


void HdOnyxRenderDelegate::_Initialize()
{
    std::cout << "[hdOnyx] Inicjalizacja Render Delegate" << std::endl;
    m_ResourceRegistry = std::make_shared<HdResourceRegistry>();
    m_RendererBackend = std::make_shared<OnyxRenderer>();

    m_BackgroundRenderThread = std::make_unique<HdRenderThread>();

    // Backend silnika tworzy Embree device który jest wymagany do tworzenia
    // zasobów biblioteki Embree. Pobieramy wskaźnik i przekazujemy go podczas
    // synchronizacji obiektów prim.
    m_RenderParam = std::make_shared<HdOnyxRenderParam>(
        m_RendererBackend->GetEmbreeDeviceHandle(),
        m_RendererBackend.get()
    );

    m_BackgroundRenderThread->SetRenderCallback(std::bind(&HdOnyxRenderDelegate::_RenderCallback, this));
    m_BackgroundRenderThread->StartThread();
}


void HdOnyxRenderDelegate::_RenderCallback()
{
    m_RendererBackend->MainRenderingEntrypoint(m_BackgroundRenderThread.get());
}


HdOnyxRenderDelegate::~HdOnyxRenderDelegate()
{
    m_BackgroundRenderThread->StopThread();

    m_BackgroundRenderThread.reset();
    m_ResourceRegistry.reset();
    m_RendererBackend.reset();
    std::cout << "[hdOnyx] Destrukcja Render Delegate" << std::endl;
}


bool HdOnyxRenderDelegate::IsPauseSupported() const
{
    return true;
}


bool HdOnyxRenderDelegate::Pause()
{
    m_BackgroundRenderThread->PauseRender();

    return m_BackgroundRenderThread->IsPauseRequested();
}


bool HdOnyxRenderDelegate::Resume()
{
    m_BackgroundRenderThread->ResumeRender();

    return m_BackgroundRenderThread->IsRendering();
}


HdResourceRegistrySharedPtr HdOnyxRenderDelegate::GetResourceRegistry() const
{
    return m_ResourceRegistry;
}

void HdOnyxRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
    std::cout << "[hdOnyx] => CommitResources RenderDelegate" << std::endl;
}

HdRenderPassSharedPtr HdOnyxRenderDelegate::CreateRenderPass(HdRenderIndex *index, HdRprimCollection const& collection)
{
    std::cout << "[hdOnyx] RenderPass | Collection = " << collection.GetName() << std::endl;

    return HdRenderPassSharedPtr {
        new HdOnyxRenderPass(index, collection, m_RendererBackend, m_BackgroundRenderThread.get())
    };
}

HdRprim *HdOnyxRenderDelegate::CreateRprim(TfToken const& typeId, SdfPath const& rprimId)
{
    std::cout << "Create Tiny Rprim type=" << typeId.GetText() 
        << " id=" << rprimId 
        << std::endl;

    if (typeId == HdPrimTypeTokens->mesh)
    {
        return new HdOnyxMesh(rprimId);
    }

    TF_CODING_ERROR("Unknown Rprim type=%s id=%s",
        typeId.GetText(),
        rprimId.GetText());

    return nullptr;
}

void HdOnyxRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    std::cout << "Destroy Tiny Rprim id=" << rPrim->GetId() << std::endl;
    delete rPrim;
}

HdSprim *HdOnyxRenderDelegate::CreateSprim(TfToken const& typeId,
                                    SdfPath const& sprimId)
{
    // Niezbędne do otrzymania poprawnych danych z UsdImagingGLEngine
    // w RenderPass.
    if (typeId == HdPrimTypeTokens->camera) return new HdCamera(sprimId);

    TF_CODING_ERROR("Unknown Sprim type=%s id=%s",
        typeId.GetText(),
        sprimId.GetText());
    return nullptr;
}

HdSprim *HdOnyxRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Creating unknown fallback sprim type=%s",
        typeId.GetText());
    return nullptr;
}

void
HdOnyxRenderDelegate::DestroySprim(HdSprim *sPrim)
{
    TF_CODING_ERROR("Destroy Sprim not supported");
}

HdBprim *
HdOnyxRenderDelegate::CreateBprim(TfToken const& typeId, SdfPath const& bprimId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer)
    {
        return new HdOnyxRenderBuffer(bprimId);
    }

    TF_CODING_ERROR("Unknown Bprim type=%s id=%s",
        typeId.GetText(),
        bprimId.GetText());
    return nullptr;
}

HdBprim *
HdOnyxRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Creating unknown fallback bprim type=%s",
        typeId.GetText());
    return nullptr;
}

void
HdOnyxRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    TF_CODING_ERROR("Destroy Bprim not supported");
}

HdInstancer *
HdOnyxRenderDelegate::CreateInstancer(
    HdSceneDelegate *delegate,
    SdfPath const& id)
{
    TF_CODING_ERROR("Creating Instancer not supported id=%s",
        id.GetText());
    return nullptr;
}

void
HdOnyxRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    TF_CODING_ERROR("Destroy instancer not supported");
}

HdRenderParam *HdOnyxRenderDelegate::GetRenderParam() const
{
    return m_RenderParam.get();
}


HdAovDescriptor HdOnyxRenderDelegate::GetDefaultAovDescriptor(const TfToken& aovName) const
{
    if (aovName == HdAovTokens->color)
    {
        return HdAovDescriptor(HdFormatUNorm8Vec4, true, VtValue(GfVec4f(0.0f)));
    }

    if (aovName == HdAovTokens->depth)
    {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(1.0f));
    }

    if (aovName == HdAovTokens->normal)
    {
        return HdAovDescriptor(HdFormatUNorm8Vec4, false, VtValue(GfVec4f(0.0f)));
    }

    return HdAovDescriptor();
}


PXR_NAMESPACE_CLOSE_SCOPE
