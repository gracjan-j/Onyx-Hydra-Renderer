#pragma once

#include <optional>

#include <Views/View.h>

#include <ImGui/imgui.h>
#include <ImGui/backends/imgui_impl_opengl3.h>
#include <ImGui/backends/imgui_impl_glfw.h>

#include <GLFW/glfw3.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>

#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/imaging/glf/drawTarget.h>


namespace GUI {


class HydraRenderView: public View
{
public:

    HydraRenderView() {};
    ~HydraRenderView() {};

    bool Initialise() override;

    bool PrepareBeforeDraw() override;

    void Draw() override;

    bool SetNewSize() override {};
    
    bool CreateOrUpdateDrawTarget(int resolutionX, int resolutionY);
    bool CreateOrUpdateImagingEngine(pxr::UsdStageRefPtr stage);

    GLuint m_ColorTextureID = 0;
    
private:
    
    std::optional<pxr::UsdStageRefPtr> m_Stage;
    std::optional<pxr::GlfDrawTargetRefPtr> m_UsdDrawTarget;
    std::optional<pxr::UsdImagingGLEngine*> m_UsdImagingEngine;
    
    std::vector<pxr::UsdPrim> m_StageCameraVector;
    
    const std::optional<std::string> m_GuiDisplayName = std::make_optional("Hydra Render View");
};


}
