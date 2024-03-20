#include "Views/MenuView.h"

#include <imgui_internal.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <nfd.hpp>


namespace GUI
{


bool MenuView::Initialise()
{

}


bool MenuView::PrepareBeforeDraw()
{
    // Pobieramy rozmiar renderowalnej cześci okna.
    ImGuiViewport *mainImGuiViewport = ImGui::GetMainViewport();
    auto menuViewSize = mainImGuiViewport->WorkSize;

    // Ustawiamy szerokość widoku na 1/4 szerokości roboczej okna aplikacji.
    menuViewSize.x = menuViewSize.x * 0.25;
    // Ustawiamy wysokość widoku na równy wysokości roboczej okna aplikacji.
    // - Komponent Y nie wymaga zmian.

    // Określamy rozmiar następnego tworzonego okna.
    ImGui::SetNextWindowSize(menuViewSize);
    // Zakotwiczmy okno w lewym górnym rogu (pozycja startowa 0.0, 0.0)
    ImGui::SetNextWindowPos(mainImGuiViewport->WorkPos);

    // Używamy dodatkowych opcji stylowania okna.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 2.5f));
}

void TextCentered(std::string text)
{
    float windowWidth = ImGui::GetWindowSize().x;
    float textWidth = ImGui::CalcTextSize(text.c_str()).x;

    float windowCenteredTextWidth = (windowWidth - textWidth) / 2.0;

    float min_indentation = 20.0f;
    if (windowCenteredTextWidth <= min_indentation) {
        windowCenteredTextWidth = min_indentation;
    }

    ImGui::SameLine(windowCenteredTextWidth);
    ImGui::PushTextWrapPos(windowWidth - windowCenteredTextWidth);
    ImGui::TextWrapped(text.c_str());
    ImGui::PopTextWrapPos();
}


void MenuView::DrawUSDSection()
{
    if (ImGui::CollapsingHeader("USD"))
    {
        ImGui::SeparatorText("Version");
        ImGui::BulletText("OpenUSD %d.%d", PXR_MINOR_VERSION, PXR_PATCH_VERSION);

        ImGui::SeparatorText("Loading");
        if (ImGui::Button("Load Stage"))// Buttons return true when clicked (most widgets return true when edited/activated)
        {
            // Zmienna która otrzyma ścieżkę do pliku, jeśli operacja się powiedzie.
            nfdchar_t *outputFilePath;

            // Lista akceptowanych rozszerzeń pliku.
            nfdfilteritem_t filterItem[1] = {{ "USD", "usd, usdz, usdc, usda" }};

            nfdresult_t fileBrowserResult = NFD_OpenDialog(&outputFilePath, filterItem, 1, "~/Downloads");
            if (fileBrowserResult == NFD_OKAY)
            {
                // Próbujemy otworzyć scenę z pliku USD.
                m_SelectedStage = pxr::UsdStage::Open(outputFilePath);

                //Jesteśmy odpowiedzialni za wywołanie tej funkcji jeśli operacja się powiodła
                // i ścieżka została zwrócona.
                NFD_FreePath(outputFilePath);
            }
            else if (fileBrowserResult == NFD_CANCEL)
            {
                std::cout << "Użytkownik nie wybrał pliku." << std::endl;
            }
            else
            {
                std::cout << "[Error] Przeglądarka plików zwróciła błąd: " << NFD_GetError() << std::endl;
            }
        }
        if (m_SelectedStage)
        {
            ImGui::SeparatorText("Stage Details");
            ImGui::BulletText("Name: %s", m_SelectedStage->GetRootLayer()->GetDisplayName().c_str());
        }
    }
}


void MenuView::Draw()
{
    // Zablokowane zostają:
    // - Możliwość poruszania, zamykania
    constexpr ImGuiWindowFlags renderWindowFlags =
        ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoResize;

    ImGui::Begin(m_GuiDisplayName.value_or("View").c_str(), nullptr, renderWindowFlags);
    ImGui::Spacing();

    DrawUSDSection();

    // Zrzucamy poprzednio dodane style ze stosu.
    ImGui::PopStyleVar(3);
    ImGui::End();
}


}
