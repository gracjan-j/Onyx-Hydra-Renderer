add_subdirectory(GUI)

# Dodajemy "target" pliku wykonywalnego w formacie macOS bundle
# by wymusić stworzenie pliku .app przez CMake zamiast surowego pliku binarnego.
add_executable(OnyxApp MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(OnyxApp PRIVATE
    # Target ImGui uwzględnia GLFW oraz OpenGL
    OnyxGUI
)
