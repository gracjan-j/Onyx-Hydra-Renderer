#include "../include/material.h"

#include <iostream>
#include <pxr/imaging/hd/sceneDelegate.h>

#include "renderParam.h"

PXR_NAMESPACE_OPEN_SCOPE

// Definiujemy kilka wymaganych tokenó które zdają się nie być zdefiniowane
// w plikach USD. Jeśli token już istnieje, USD zadba o to żeby wykorzystać istniejący token.
TF_DEFINE_PRIVATE_TOKENS(m_PrivateTokens,
    (diffuseColor)
    (UsdPreviewSurface)
    (ior)
);



HdOnyxMaterial::HdOnyxMaterial(SdfPath const& id)
:HdMaterial(id)
{}


HdDirtyBits HdOnyxMaterial::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}


void HdOnyxMaterial::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits)
{
    if (*dirtyBits == Clean) return;

    // Pobieramy unikalne ID prima typu Mesh
    auto& primID = GetId();

    // Flaga wskazująca na to czy mesh został zsynchronizowany pierwszy raz.
    bool newMaterial = false;

    auto* onyxRenderParam = static_cast<HdOnyxRenderParam*>(renderParam);

    // Warunek będzie prawdziwy tylko dla całkowicie nowych obiektów
    if (*dirtyBits == AllDirty)
    {
        std::cout << "[hdOnyx] - Utworzono nowy materiał: " << primID.GetString() << std::endl;
        newMaterial = true;
    }

    // Materiały w USD są reprezentowane jako sieć połączeń komórek (nodes).
    // Pobieramy sieć materiału.
    VtValue materialGraphValue = sceneDelegate->GetMaterialResource(GetId());

    // Jeżeli materiał nie zawiera poprawnej sieci.
    if (!materialGraphValue.IsHolding<HdMaterialNetworkMap>())
    {
        *dirtyBits = Clean;
        return;
    }

    // Odpakowujemy sieć z kontenera VtValue wiedząc że zawiera sieć.
    const HdMaterialNetworkMap &oldMaterialGraph = materialGraphValue.UncheckedGet<HdMaterialNetworkMap>();

    // Konwertujemy stary typ sieci do nowego (sugestia z dokumentacji OpenUSD).
    HdMaterialNetwork2 materialGraph = HdConvertToHdMaterialNetwork2(oldMaterialGraph, nullptr);

    // Szukamy połączenia "surface" które jest wyjściem materiału dla standardowej.
    auto surfaceOutput = materialGraph.terminals.find(HdMaterialTerminalTokens->surface);

    // Brak połączenia
    if (surfaceOutput == materialGraph.terminals.end())
    {
        *dirtyBits = Clean;
        return;
    }

    // Szukamy komórki która jest podłączona do wyjścia materiału z mapy dostępnych komórek.
    std::optional<HdMaterialNode2> previewSurfaceNode;
    for (auto& node : materialGraph.nodes)
    {
        // Pomijamy niepasujące komórki
        if(node.first != surfaceOutput->second.upstreamNode)
        {
            continue;
        }

        // Jeśli znajdziemy komórkę, sprawdzamy czy ma odpowiedni typ
        if (node.second.nodeTypeId != m_PrivateTokens->UsdPreviewSurface)
        {
            *dirtyBits = Clean;
            return;
        }

        previewSurfaceNode = node.second;
    }

    pxr::GfVec3f diffuseColor;
    float indexOfRefraction;

    for (auto& nodeParameter : previewSurfaceNode->parameters)
    {
        if(nodeParameter.first == m_PrivateTokens->diffuseColor)
        {
            diffuseColor = nodeParameter.second.GetWithDefault<GfVec3f>(GfVec3f{0.18, 0.18, 0.18});
        }
        else if(nodeParameter.first == m_PrivateTokens->ior)
        {
            indexOfRefraction = nodeParameter.second.GetWithDefault<float>(1.5);
        }
    }

    // Jeśli dotarliśmy tutaj, pobraliśmy dane materiału.
    onyxRenderParam->GetRendererHandle()->AttachOrUpdateMaterial(
        diffuseColor, indexOfRefraction, primID, newMaterial
    );


    *dirtyBits = Clean;
}



PXR_NAMESPACE_CLOSE_SCOPE
