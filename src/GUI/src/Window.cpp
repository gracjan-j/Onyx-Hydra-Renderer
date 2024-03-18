#include <Window.h>

#include <nfd.hpp>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/imaging/glf/contextCaps.h>

#define GL_SILENCE_DEPRECATION

namespace GUI
{

    Window::Window()
    {
        // Inicjalizacja GLFW (okna) musi nastąpić przed inicjalizacją ImGui.
        InitialiseGLFW();
        InitialiseImGui();

        pxr::GlfContextCaps::InitInstance();

        m_HydraRenderView = std::make_unique<HydraRenderView>();
    }

    Window::~Window()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // Deinicjalizacja NativeFileDialog powinna nastąpić przed GLFW terminate.
        NFD_Quit();

        // Funkcja zwalnia pamięć okna.
        glfwDestroyWindow(m_WindowBackendGLFW.value());
        glfwTerminate();
    }

    bool Window::InitialiseImGui()
    {
        // Tworzymy nowy kontekst renderowania dla ImGui.
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // Wymaga stworzonego kontekstu.
        // Struktura służąca do konfiguracji operacji input-output.
        ImGuiIO& io = ImGui::GetIO();

        // Wnioskujemy o wsparcie dla zdarzeń wywołanych klawiszami klawiatury
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Ustawiamy domyślny, ciemny motyw interfejsu użytkownika.
        ImGui::StyleColorsDark();

        if (!m_WindowBackendGLFW.has_value())
        {
            std::cout << "Error: Inicjalizacja ImGui wymaga pomyślnej wcześniejszej inicjalizacji GLFW." << std::endl;
            return false;
        }

        // Finalnie, inicjalizujemy ImGui za pomocą wcześniej utworzonego okna.
        ImGui_ImplGlfw_InitForOpenGL(m_WindowBackendGLFW.value(), true);

        // Wersja 1.50 języka GLSL jest wymagana do poprawnego działania modułów OpenUSD
        // opierających się na OpenGL.
        const std::string shadingLanguageVersion = "#version 150";
        ImGui_ImplOpenGL3_Init(shadingLanguageVersion.c_str());

        return true;
    }

    bool Window::InitialiseGLFW()
    {
        // MARK: - Inicjalizacja GLFW
        //
        // Funkcja wymaga wskaźnika do funkcji (int, const char*)->void
        // której zostaną przekazane dane potencjalnego błędu inicjalizacji OpenGL.
        glfwSetErrorCallback(ErrorCallbackFunctionGLFW);

        // Inicjalizacja GLFW nie powiodła się, funkcja pomocnicza wypisze potencjalne błędy
        // za pomocą funkcji callback `ErrorCallbackFunctionGLFW`, nam nie pozostaje nic innego
        // jak przedwczesne wyjście z kodem błędu.
        if (!glfwInit())
        {
            return false;
        }

        // Inicjalizujemy OpenGL 3.2 z wersją języka cieniowania GLSL 150
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        // Tryb kompatybilności wymagany na macOS.
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

        // Tworzymy okno o domyślnej rozdzielczości.
        m_WindowBackendGLFW = glfwCreateWindow(1280, 720, "Onyx Pathtracer", nullptr, nullptr);

        // Błąd inicjalizacji okna, wczesne wyjście.
        if (!m_WindowBackendGLFW.has_value())
        {
            return false;
        }

        // Tworzymy kontekst renderowania w oknie.
        glfwMakeContextCurrent(m_WindowBackendGLFW.value());
        // Włączamy synchronizację pionową.
        // To musi nastąpić po stworzeniu kontekstu renderowania!
        glfwSwapInterval(1);

        // Inicjalizujemy Native File Dialog po inicjalizacji GLFW wg. zaleceń dokumentacji
        if (NFD_Init() != NFD_OKAY) {
            return false;
        }

        return true;
    }

    void Window::RunRenderingLoop()
    {
        // Main loop
        while (!glfwWindowShouldClose(m_WindowBackendGLFW.value()))
        {
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Przygotowujemy render klatki przed wyświetleniem
            // Rozmiar renderu powinien odpowiadać rozmiarowi okna.
            ImVec2 viewportSize = ImGui::GetMainViewport()->WorkSize;
            m_HydraRenderView->CreateOrUpdateDrawTarget(viewportSize.x, viewportSize.y);
            m_HydraRenderView->PrepareBeforeDraw();
            m_HydraRenderView->Draw();

            if (m_ShowDemoWidget) ImGui::ShowDemoWindow(&m_ShowDemoWidget);

            {
                ImGui::Begin("Onyx");

                ImGui::Text("STAGE");
                
                if (ImGui::Button("Otwórz USD"))// Buttons return true when clicked (most widgets return true when edited/activated)
                {
                    // Zmienna która otrzyma ścieżkę do pliku, jeśli operacja się powiedzie.
                    nfdchar_t *outputFilePath;
                    
                    // Lista akceptowanych rozszerzeń pliku.
                    nfdfilteritem_t filterItem[1] = {{ "USD", "usd, usdz, usdc, usda" }};
                    
                    nfdresult_t fileBrowserResult = NFD_OpenDialog(&outputFilePath, filterItem, 1, "~/Downloads");
                    if (fileBrowserResult == NFD_OKAY)
                    {
                        // Próbujemy otworzyć scenę z pliku USD.
                        m_OpenedStage = pxr::UsdStage::Open(outputFilePath);

                        if (m_OpenedStage.has_value())
                        {
                            m_HydraRenderView->CreateOrUpdateImagingEngine(m_OpenedStage.value());
                            m_HydraRenderView->Initialise();
                        }
                        
                        // Jesteśmy odpowiedzialni za wywołanie tej funkcji jeśli operacja się powiodła
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

                ImGui::Checkbox("Demo Window", &m_ShowDemoWidget);

                ImGui::End();
            }

            ImGui::Render();

            // Pobieramy rozmiar okna z aktualnego framebuffera GLFW
            int windowWidth, windowHeight;
            glfwGetFramebufferSize(m_WindowBackendGLFW.value(), &windowWidth, &windowHeight);
            
            // Czyścimy okno domyślnym kolorem tła.
            glViewport(0, 0, windowWidth, windowHeight);
            glClearColor(m_WindowBackground.x, m_WindowBackground.y, m_WindowBackground.z, m_WindowBackground.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Podmieniamy framebuffer (korzystamy z double-buffering) aby zminimalizować
            // przerwy między klatkami.
            glfwSwapBuffers(m_WindowBackendGLFW.value());
        }
    }


}
