#include "rendererPlugin.h"
#include "renderDelegate.h"

#include "pxr/imaging/hd/rendererPluginRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE


// Rejestrujemy plugin który ładuje silnik w rejestrze pluginów OpenUSD.
// To pozwala USD identyfikację silnika za pomocą tokenu.
TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdOnyxRendererPlugin>();
}


HdRenderDelegate* HdOnyxRendererPlugin::CreateRenderDelegate()
{
    return new HdOnyxRenderDelegate();
}


HdRenderDelegate* HdOnyxRendererPlugin::CreateRenderDelegate(HdRenderSettingsMap const& settingsMap)
{
    return new HdOnyxRenderDelegate(settingsMap);
}


void HdOnyxRendererPlugin::DeleteRenderDelegate(HdRenderDelegate *renderDelegate)
{
    delete renderDelegate;
}


bool HdOnyxRendererPlugin::IsSupported(bool gpuEnabled) const
{
    // Silnik nie wykonuje obliczeń na karcie graficznej.
    // Korzysta jedynie z CPU, tak więc wsparcie sprzętowe GPU
    // nie wpływa na wsparcie silnika w systemie.
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
