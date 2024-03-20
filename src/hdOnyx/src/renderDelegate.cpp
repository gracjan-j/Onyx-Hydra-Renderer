#include "renderDelegate.h"
#include "mesh.h"
#include "renderPass.h"

#include <pxr/imaging/hd/renderBuffer.h>

#include <iostream>

#include "renderBuffer.h"

PXR_NAMESPACE_OPEN_SCOPE
    const TfTokenVector HdOnyxRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
};


const TfTokenVector HdOnyxRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    
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
}


HdOnyxRenderDelegate::~HdOnyxRenderDelegate()
{
    m_ResourceRegistry.reset();
    std::cout << "[hdOnyx] Destrukcja Render Delegate" << std::endl;
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

    return HdRenderPassSharedPtr(new HdOnyxRenderPass(index, collection));
}

HdRprim *HdOnyxRenderDelegate::CreateRprim(TfToken const& typeId, SdfPath const& rprimId)
{
    std::cout << "Create Tiny Rprim type=" << typeId.GetText() 
        << " id=" << rprimId 
        << std::endl;

    if (typeId == HdPrimTypeTokens->mesh) {
        return new HdOnyxMesh(rprimId);
    } else {
        TF_CODING_ERROR("Unknown Rprim type=%s id=%s", 
            typeId.GetText(), 
            rprimId.GetText());
    }
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
    return nullptr;
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

    return HdAovDescriptor();
}


PXR_NAMESPACE_CLOSE_SCOPE
