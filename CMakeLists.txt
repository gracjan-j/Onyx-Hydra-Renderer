# Minimalna wersja wymagana do wsparcia architektury ARM64 na macOS
cmake_minimum_required (VERSION 3.19)

# Tworzymy projekt Cmake
project(Onyx VERSION 1.0)

# Wymagamy C++17 dla OpenUSD
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(extern)

# Dodajemy "target" pliku wykonywalnego
add_executable(OnyxApp src/main.cpp)

target_link_libraries(OnyxApp PRIVATE
    ImGui
)

