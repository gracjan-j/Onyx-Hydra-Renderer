# Minimalna wersja wymagana do wsparcia architektury ARM64 na macOS
cmake_minimum_required (VERSION 3.19)

# Tworzymy projekt Cmake
project(Onyx VERSION 1.1)

# Wymagamy C++17 dla OpenUSD
set(CMAKE_CXX_STANDARD 17)

# Dodajemy wszystkie wymagane zewnętrzne biblioteki.
add_subdirectory(extern)

# Dodajemy "target" pliku wykonywalnego w formacie macOS bundle
# by wymusić stworzenie pliku .app przez CMake zamiast surowego pliku binarnego.
add_executable(OnyxApp MACOSX_BUNDLE
    src/main.cpp
)

target_link_libraries(OnyxApp PRIVATE
    # Target ImGui uwzględnia GLFW oraz OpenGL
    ImGui
)

