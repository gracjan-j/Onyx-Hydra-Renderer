#include <Views/HydraRenderView.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>


bool GUI::HydraRenderView::Initialise()
{
    // Jeśli wektor zawiera stare dane, czyścimy go.
    if (!m_StageCameraVector.empty())
    {
        m_StageCameraVector.clear();
    }
    
    // Wykrywamy dostępne kamery w scenie.
    pxr::UsdPrimRange scenePrimRange = m_Stage.value()->Traverse();
    
    // Iterujemy przez wszystkie obiekty "prim" w scenie
    for (auto iteratorBegin = scenePrimRange.begin(); iteratorBegin != scenePrimRange.end(); iteratorBegin++)
    {
        // Jeśli typ prim'a jest pochodnym typu "Camera".
        if(iteratorBegin->IsInFamily(pxr::TfToken{"Camera"}))
        {
            // Dodajemy go do wektora.
            m_StageCameraVector.emplace_back(*iteratorBegin);
        }
    }
    
    if (m_StageCameraVector.empty())
    {
        return false;
    }
    
    return true;
}


bool GUI::HydraRenderView::CreateOrUpdateDrawTarget(int resolutionX, int resolutionY)
{
    // Jeśli aktualny deskryptor istnieje, sprawdzamy czy wymaga aktualizacji (rozmiar)
    if (m_UsdDrawTarget.has_value())
    {
        pxr::GfVec2i currentTargetSize = m_UsdDrawTarget.value()->GetSize();
        pxr::GfVec2i newTargetSize = pxr::GfVec2i(resolutionX, resolutionY);

        if (currentTargetSize != newTargetSize)
        {
            // Rozmiar tekstury wymaga aktualizacji.
            m_UsdDrawTarget.value()->Bind();
            m_UsdDrawTarget.value()->SetSize(newTargetSize);
            m_UsdDrawTarget.value()->Unbind();

            return true;
        }
    }
    
    auto drawTargetTextureSize = pxr::GfVec2i(resolutionX, resolutionY);
    
    // Tworzymy destynację renderowania za pomocą modułu GL w OpenUSD.
    // DrawTarget to deskryptor który może zawierać więcej niż jedną teksturę
    // które są argumentami wejściowymi do przejść na GPU.
    // Wszystkie tekstury będą tworzone w tym samym rozmiarze.
    m_UsdDrawTarget = pxr::GlfDrawTarget::New(drawTargetTextureSize);
    
    // Jeśli nie udało się utworzyć deskryptora, zwracamy błąd
    if (!m_UsdDrawTarget.has_value())
    {
        return false;
    }
    
    // Ustawiamy deskryptor jako aktualny.
    m_UsdDrawTarget.value()->Bind();
    
    // Tworzymy wymagane tekstury które będą argumentami wejściowymi do renderowania.
    //
    // Tesktura "color" jest teksturą o formacie RGBA która przechowuje pełny wynik renderowania
    m_UsdDrawTarget.value()->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
    // Tekstura "depth" jest teskturą pomocniczą która zawiera wartości głębi.
    m_UsdDrawTarget.value()->AddAttachment("depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32);
    m_ColorTextureID = m_UsdDrawTarget.value()->GetAttachment("color")->GetGlTextureName();

    // Tymczasowo wyłączamy deskryptor z użytku.
    m_UsdDrawTarget.value()->Unbind();

    return true;
}


bool GUI::HydraRenderView::CreateOrUpdateImagingEngine(pxr::UsdStageRefPtr stage)
{
    if (m_Stage == stage)
    {
        return true;
    }

    m_Stage.reset();
    m_Stage = stage;

    Initialise();
    
    // Tworzymy obiekt wymiany informacji między interfejsem a silnikiem dla nowej sceny.
    m_UsdImagingEngine = new pxr::UsdImagingGLEngine(m_Stage.value()->GetPseudoRoot().GetPath(), pxr::SdfPathVector());
    
    // Mówimy silnikowi że oczekujemy zwrócenia pełnego wyniku renderowania.
    // Możliwe jest także wnioskowanie o inne typy danych (np. do debugowania danych)
    // takie jak "normal" czy "depth".
    // Ważne, żeby wybrany silnik wspierał renderowanie wybranego typu danych.
    m_UsdImagingEngine.value()->SetRendererAov(pxr::TfToken("color"));
    
    // Istotna flaga która wnioskuje o to, żeby pośrednik przeprowadzał tzw. "Blit Render Encoding"
    // który jest operacją kopiującą dane z wewnętrznej tymczasowej tekstury pośrednika
    // do aktualnie przypisanego "framebuffera" czyli deskryptora tekstur.
    // Przed każdym wywołaniem "Render" musimy podpiąć własny "framebuffer" do którego wykonana zostanie kopia.
    m_UsdImagingEngine.value()->SetEnablePresentation(true);


    auto availableRenderersVector = m_UsdImagingEngine.value()->GetRendererPlugins();

    for (auto& plugin: availableRenderersVector)
    {
        std::cout << plugin.GetString() << std::endl;
    }
    
    m_UsdImagingEngine.value()->SetRendererPlugin(availableRenderersVector.back());

    return true;
}


bool GUI::HydraRenderView::PrepareBeforeDraw()
{
    // Jeśli "most" do silnika lub docelowa tekstura wyniku renderowania nie istnieje, zwracamy błąd.
    if (!m_UsdImagingEngine.has_value())
    {
        return false;
    }

    // Rozmiar renderu powinien odpowiadać rozmiarowi okna.
    ImVec2 viewportSize = ImGui::GetMainViewport()->WorkSize;
    CreateOrUpdateDrawTarget(viewportSize.x, viewportSize.y);
    
    // Przed każdym rysowaniem widoku musimy wyrenderować klatkę z silnika za pomocą frameworku Hydra,
    // który jest pośrednikiem pomiędzy interfejsem użytkownika a silnikiem renderującym w OpenUSD.
    // Podczas operacji "Draw" widoku korzystamy już z gotowej tesktury która zostaje wyświetlona w interfejsie.
    
    // Ustawiamy teksturę jako aktualny "framebuffer". Framebuffer jest obiektem
    // do którego wpisywany jest wynik podczas wykonywania operacji renderowania przejścia na GPU.
    // USD uruchamia "Blit Render Encoder" który kopiuje dane z wewnętrznej tekstury do aktualnego framebuffera,
    // gdzie wewnętrzna tesktura jest obiektem do którego tymczasowo wpisywał dane wybrany silnik renderujący.
    m_UsdDrawTarget.value()->Bind();

    auto renderParams =  pxr::UsdImagingGLRenderParams();
    renderParams.frame = 0;
    renderParams.clearColor = pxr::GfVec4f(0.0, 0.0, 0.0, 1.0);
    renderParams.enableLighting = true;
    renderParams.enableSceneLights = true;

    // Wnioskujemy o wykonanie renderu pojedynczej klatki przez wybrany silnik renderowania.
    pxr::GfVec4d viewport(0, 0, m_UsdDrawTarget.value()->GetSize()[0], m_UsdDrawTarget.value()->GetSize()[1]);
    m_UsdImagingEngine.value()->SetRenderViewport(viewport);

    if (!m_StageCameraVector.empty())
    {
        m_UsdImagingEngine.value()->SetCameraPath(m_StageCameraVector[0].GetPath());
    }

    m_UsdImagingEngine.value()->Render(m_Stage.value()->GetPseudoRoot(), renderParams);

    // Wyłączamy teskturę z użytku. Operacja renderowania wywołana powyżej została zarejestrowana.
    // Nie chcemy, żeby zewnętrzne przejścia nadpisały zawartość tesktury z wynikiem renderowania aktualnej klatki.
    m_UsdDrawTarget.value()->Unbind();
}


void GUI::HydraRenderView::Draw()
{
    // Tworzone okno ImGui jest oknem o rozmiarze okna aplikacji.
    // Zablokowane zostają:
    // - Możliwość poruszania, zamykania, minimalizowania, zmiany rozmiaru, wyciągania na wierzch
    // - Wyświetlanie tytułu okna
    // W ten sposób wynik renderowania zawsze pozostaje tłem aplikacji.
    constexpr ImGuiWindowFlags renderWindowFlags =
        ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoTitleBar;

    // Pobieramy rozmiar renderowalnej cześci okna.
    ImGuiViewport *mainImGuiViewport = ImGui::GetMainViewport();
    
    // Ustawiamy rozmiar okna na równy powierzchni roboczej okna aplikacji.
    ImGui::SetNextWindowSize(mainImGuiViewport->WorkSize);
    ImGui::SetNextWindowPos(mainImGuiViewport->WorkPos);
    
    // Okno nie powinno wspierać przezroczystości.
    ImGui::SetNextWindowBgAlpha(0.0);
    
    // Wyłączamy wszelkie opcje stylowania.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    // Tworzymy zawsze widoczne okno.
    ImGui::Begin(m_GuiDisplayName.value().c_str(), nullptr, renderWindowFlags);
    
    // Wyświetlamy teksturę z wynkikiem renderowania.
    if (m_UsdDrawTarget.has_value())
    {
        auto getImageSize = m_UsdDrawTarget.value()->GetSize();
        ImGui::Image((void*)(intptr_t)m_ColorTextureID, ImVec2(getImageSize[0], getImageSize[1]), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
    }

    ImGui::PopStyleVar(3);
    ImGui::End();
}
