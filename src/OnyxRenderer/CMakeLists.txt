# Wczytujemy funkcje CMake służące do pobrania plików.
include(FetchContent)

# Pobieramy bibliotekę Embree budowaną dla architektury macOS (ARM64).
# Biblioteka jest używana do budowania hierarchicznych struktur przyspieszenia
# (BVH/RTAS - Bounding Volume Hierarchy / Raytracing Acceleration Structure)
# służących do szukania intersekcji pomiędzy promieniem a hierarchią trójkątów
# z których składa się geometria.
FetchContent_Declare(
    Embree_v4
    URL https://github.com/embree/embree/releases/download/v4.3.1/embree-4.3.1.arm64.macosx.zip
)

# Wykonanie funkcji zapewnia dostępność danych.
FetchContent_MakeAvailable(Embree_v4)

# Funkcja automatycznie znajdzie i zainicjalizuje targety Embree
# z pobranego folderu.
find_package(embree REQUIRED HINTS ${embree_SOURCE_DIR} NAMES embree)

# -----------------------------
# Onyx Renderer
#
# Silnik jest tworzony jako osobna biblioteka z której będzie korzystać
# Hydra Render Delegate o nazwie hdOnyx. Delegate wysyła żądania do silnika
# w celu utworzenia/modyfikacji/usunięcia zasobów, a także w celu wyrenderowania
# obrazu do przekazanego bufora na dane pikseli.
#
# Rozdzielenie silnika oraz Hydra Render Delegate pozwala na interakcję z silnikiem
# bez udziału OpenUSD.
add_library(OnyxRenderer)


set(ONYX_RENDER_HEADERS
    include/OnyxRenderer.h
)

set(ONYX_RENDER_SOURCES
    src/OnyxRenderer.cpp
)

target_link_libraries(OnyxRenderer
    PRIVATE
    embree
)

target_sources(OnyxRenderer PRIVATE
    ${ONYX_RENDER_SOURCES}
    ${ONYX_RENDER_HEADERS}
)

target_include_directories(OnyxRenderer
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
PRIVATE
    include
)

