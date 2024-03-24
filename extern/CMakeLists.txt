# ZDEFINIOWANE TARGETY:
# - glfw
# - ImGui

# GLFW jest wymaganym "backendem" to stworzenia okna oraz obsługi urządzeń wejściowych przez ImGui.
# W tym celu, dodajemy submoduł GLFW do naszego projektu. GLFW jest biblioteką z przygotowanym CMakeLists.
add_subdirectory(GLFW)

add_subdirectory(NativeFileDialogExtended)

# ------------------------------------------
# ImGui nie posiada gotowego pliku CMakeList.
# Tworzymy własny "target" modułu do którego będziemy linkować z głównego pliku wykonywalnego aplikacji.
# Uwzględniamy jedynie niezbędne komponenty biblioteki ImGui.

# Definiujemy target ImGui.
add_library(ImGui)

# Zmienna pomocnicza ścieżki do submodułu ImGui.
set(IMGUI_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)

# Dodajemy wymagane pliki źródłowe
target_sources(ImGui PRIVATE
    ${IMGUI_ROOT}/imgui.cpp
    ${IMGUI_ROOT}/imgui_draw.cpp
    ${IMGUI_ROOT}/imgui_widgets.cpp
    ${IMGUI_ROOT}/imgui_tables.cpp

    # Pierwotny commit korzysta z demo w celu przetestowania
    # działania ImGui oraz GLFW.
    ${IMGUI_ROOT}/imgui_demo.cpp

    # Backend renderowania - wymagane do utworzenia okna i wyświetlania rezultatu
    # Korzystamy z GLFW + OpenGL które są opisywane przez dokumentację
    # ImGui jako najbardziej stabilne na macOS.
    ${IMGUI_ROOT}/backends/imgui_impl_glfw.cpp
    ${IMGUI_ROOT}/backends/imgui_impl_opengl3.cpp

    # Wsparcie dla typów z biblioteki STL języka C++
    ${IMGUI_ROOT}/misc/cpp/imgui_stdlib.cpp
)

# Dodatkowo, uzwględniamy wymagane pliki nagłówkowe
target_include_directories(ImGui
    PUBLIC
    # Używamy aktualnej scieżki do pliku CMakeList
    # aby pliki nagłówkowe ImGui były importowane w namespace ich folderu nadrzędnego
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_ROOT}

    PRIVATE
    ${IMGUI_ROOT}
)

# Linkowanie do zewnętrznych bibliotek
target_link_libraries(ImGui PUBLIC
        # GLFW jest naszym backendem tworzenia okna i obsługi inputu użytkownika
        # (myszka, klawiatura)
        glfw

        # OpenGL jest wymaganym backendem przez ImGui do zarządzania procesem renderowania GUI
        # w oknie.
        "-framework OpenGL"
)