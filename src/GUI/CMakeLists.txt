# Definiujemy moduł aplikacji odpowiedzialny za graficzny interfejs użytkownika.
add_library(OnyxGUI)

set(MODULE_SOURCES
    src/Window.cpp

    # Widoki
    src/Views/MenuView.cpp
    src/Views/HydraRenderView.cpp
)

set(MODULE_HEADERS
    include/Window.h

    # Widoki
    include/Views/View.h
    include/Views/MenuView.h
    include/Views/HydraRenderView.h
)

# Dodajemy pliki źródłowe z których składa się moduł.
target_sources(OnyxGUI PRIVATE
    ${MODULE_SOURCES}
    ${MODULE_HEADERS}
)

# Dodajemy ścieżki w których szukane są pliki nagłówkowe dla modułu.
target_include_directories(OnyxGUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include(FindPython3)
find_package(Python3 REQUIRED COMPONENTS Development)

# Linkujemy do zewnętrznych "targetów" które są wykorzystywane w module.
# Oznaczenie PUBLIC pozwala na odziedziczenie zależności przez użytkowników modułu.
target_link_libraries(OnyxGUI PUBLIC
    # Graficzny interfejs użytkownika korzysta z biblioteki ImGui.
    # Target ImGui upublicznia biblioteki OpenGL oraz GLFW.
    ImGui
    # Biblioteka pozwalająca na obsłużenie systemowego okna wyboru pliku na macOS.
    # Jedyną alternatywą dla takiego rozwiązania jest użycie języka Objective-C.
    nfd

    # Zbiór bibliotek OpenUSD zdefiniowany pod zmienną.
    ${PXR_LIBRARIES}

    ${Python3_LIBRARIES}
)