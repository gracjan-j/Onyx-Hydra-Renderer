set(PXR_PACKAGE hdOnyx)

set(HD_ONYX_SOURCES
    src/mesh.cpp
    src/light.cpp
    src/material.cpp
    src/renderPass.cpp
    src/renderBuffer.cpp
    src/renderDelegate.cpp
    src/rendererPlugin.cpp
)

set(HD_ONYX_HEADERS
    include/mesh.h
    include/light.h
    include/material.h
    include/renderPass.h
    include/renderParam.h
    include/renderBuffer.h
    include/renderDelegate.h
    include/rendererPlugin.h
)

# Korzystamy z makra zdefiniowanego dla pluginów
# tworzonych do biblioteki OpenUSD która automatyzuje
# proces konfiguracji oraz instalacji pluginu.
usd_plugin(${PXR_PACKAGE}
    RESOURCE_FILES
        # Plik opisujący specyfikacje
        # pluginu silnika.
        plugInfo.json

    LIBRARIES
        # OpenUSD - Hydra
        hd
        # OpenUSD - System tokenizacji
        tf
        OnyxRenderer

    CPPFILES
        ${HD_ONYX_SOURCES}
        ${HD_ONYX_HEADERS}

    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include

    DISABLE_PRECOMPILED_HEADERS
)
