#pragma once

#include <optional>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usd/common.h>

#include "HydraRenderView.h"
#include "View.h"


namespace GUI
{

class MenuView: public View
{
public:

    MenuView(HydraRenderView* renderView);
    ~MenuView() override = default;

    bool Initialise() override;

    bool PrepareBeforeDraw() override;

    void Draw() override;

    pxr::UsdStageRefPtr GetSelectedStage() { return m_SelectedStage; }
    pxr::TfToken GetSelectedAOV() { return m_ComboOptionsAOV[m_ComboSelectionIndexAOV]; }

private:

    void DrawUSDSceneLoadingSection();
    void DrawAOVSection();

    pxr::UsdStageRefPtr m_SelectedStage;

    const std::optional<std::string> m_GuiDisplayName = std::make_optional("MENU");

    std::vector<pxr::TfToken> m_ComboOptionsAOV = {pxr::HdAovTokens->color, pxr::HdAovTokens->normal};
    int m_ComboSelectionIndexAOV = 0;

    HydraRenderView* m_RenderView;
};


}
