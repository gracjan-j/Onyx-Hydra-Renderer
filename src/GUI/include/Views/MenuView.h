#pragma once

#include <optional>
#include <pxr/usd/usd/common.h>

#include "View.h"


namespace GUI
{

class MenuView: public View
{
public:

    MenuView() = default;
    ~MenuView() override = default;

    bool Initialise() override;

    bool PrepareBeforeDraw() override;

    void Draw() override;

    pxr::UsdStageRefPtr GetSelectedStage() { return m_SelectedStage; }

private:

    void DrawUSDSection();

    pxr::UsdStageRefPtr m_SelectedStage;

    const std::optional<std::string> m_GuiDisplayName = std::make_optional("MENU");
};


}
