#include <Window.h>

namespace GUI
{

    Window::Window()
    {
        // Inicjalizacja GLFW (okna) musi nastąpić przed inicjalizacją ImGui.
        InitialiseGLFW();
        InitialiseImGui();
    }

    Window::~Window()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

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
        (void)io;

        // Wnioskujemy o wsparcie dla zdarzeń wywołanych klawiszami klawiatury
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Ustawiamy domyślny, ciemny motyw interfejsu użytkownika.
        ImGui::StyleColorsDark();

        if (!m_WindowBackendGLFW.has_value())
        {
            std::cout << "Error: Inicjalizacja ImGui wymaga pomyślnej wcześniejszej inicjalizacji GLFW." << std::endl;
            return false;
        }

        const std::string shadingLanguageVersion = "#version 150";

        // Finalnie, inicjalizujemy ImGui za pomocą wcześniej utworzonego okna.
        ImGui_ImplGlfw_InitForOpenGL(m_WindowBackendGLFW.value(), true);
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

        return true;
    }

    void Window::RunRenderingLoop()
    {
        // Main loop
        while (!glfwWindowShouldClose(m_WindowBackendGLFW.value()))
        {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            if (m_ShowDemoWidget)
                ImGui::ShowDemoWindow(&m_ShowDemoWidget);

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                ImGui::Checkbox("Demo Window", &m_ShowDemoWidget);      // Edit bools storing our window open/close state
                ImGui::Checkbox("Another Window", &m_ShowHelperWidget);

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::ColorEdit3("clear color", (float*)&m_WindowBackground); // Edit 3 floats representing a color

                if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);
                ImGui::End();
            }

            // 3. Show another simple window.
            if (m_ShowHelperWidget)
            {
                ImGui::Begin("Another Window", &m_ShowHelperWidget);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::Text("Hello from another window!");
                if (ImGui::Button("Close Me"))
                    m_ShowHelperWidget = false;
                ImGui::End();
            }

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(m_WindowBackendGLFW.value(), &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(m_WindowBackground.x, m_WindowBackground.y, m_WindowBackground.z, m_WindowBackground.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(m_WindowBackendGLFW.value());
        }
    }


}