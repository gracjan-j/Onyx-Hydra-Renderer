# Minimalna wersja wymagana do wsparcia architektury ARM64 na macOS
cmake_minimum_required (VERSION 3.19)

# Tworzymy projekt Cmake
project(Onyx VERSION 1.1)

# Wymagamy C++17 dla OpenUSD
set(CMAKE_CXX_STANDARD 17)

# Zakładamy, że OpenUSD zostało zbudowane przed uruchomieniem konfiguracji projektu
# za pomocą wbudowanego skryptu, tworzącego pxrConfig.cmake
# który definiuje wszystkie wymagane moduły.
# Scieżka do zbudowanego USD powinna zostać podana za pomocą `USD_PATH`.
find_package(OpenUSD REQUIRED HINTS ${USD_PATH} NAMES pxr)

if (OpenUSD_FOUND)
    message(STATUS "Znaleziono OpenUSD w wersji: ${PXR_VERSION}")

    include(cmake/USDPluginTools.cmake)
endif()

# Dodajemy wszystkie wymagane zewnętrzne biblioteki.
add_subdirectory(extern)

# Dodajemy źródła programu.
# Wewnątrz folderu plików źródłowych może znajdować się plik CMakeLists.txt delegujący
# konfigurację między modułami z których składa się program.
add_subdirectory(src)
