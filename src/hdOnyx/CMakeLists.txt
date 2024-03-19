set(PXR_PACKAGE hdOnyx)

set(ONYX_SOURCES
    src/mesh.cpp
    src/renderPass.cpp
    src/renderDelegate.cpp
    src/rendererPlugin.cpp
)

set(ONYX_HEADERS
    include/mesh.h
    include/renderPass.h
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

    CPPFILES
        ${ONYX_SOURCES}
        ${ONYX_HEADERS}

    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include

    DISABLE_PRECOMPILED_HEADERS
)
