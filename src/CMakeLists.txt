# Zakładamy, że OpenUSD zostało zbudowane przed uruchomieniem konfiguracji projektu
# za pomocą wbudowanego skryptu, tworzącego pxrConfig.cmake
# który definiuje wszystkie wymagane moduły.
# Scieżka do zbudowanego USD powinna zostać podana za pomocą `USD_PATH`.
# OpenUSD jest zależnością GUI oraz hdOnyx
find_package(OpenUSD REQUIRED HINTS ${USD_PATH} NAMES pxr)
if (OpenUSD_FOUND)
    message(STATUS "Znaleziono OpenUSD w wersji: ${PXR_VERSION}")
    include(${CMAKE_SOURCE_DIR}/cmake/USDPluginTools.cmake)
endif()

# Ustawiamy ścieżki szukania bibliotek w trakcie uruchomienia programu.
# Biblioteki będą automatycznie przekopiowane z zapewnionej instalacji OpenUSD.
set(ONYX_RPATHS
    "@executable_path/../Frameworks/OpenUSD/lib;@executable_path/../Frameworks/OpenUSD/lib/usd;@executable_path/../Frameworks/OpenUSD/plugin/usd"
)
set(CMAKE_BUILD_RPATH ${ONYX_RPATHS})
set(CMAKE_INSTALL_RPATH ${ONYX_RPATHS})

# Dodajemy moduł programu odpowiedzialny za interfejs użytkownika.
add_subdirectory(GUI)
# Dodajemy backend silnika renderowania.
add_subdirectory(OnyxRenderer)
# Dodajemy frontend (Hydra) silnika renderującego który będzie ładowany przez GUI.
add_subdirectory(hdOnyx)


# Dodajemy główny "target" pliku wykonywalnego w formacie macOS bundle
# by wymusić stworzenie pliku .app przez CMake zamiast surowego pliku binarnego.
add_executable(OnyxApp MACOSX_BUNDLE)

target_link_libraries(OnyxApp PRIVATE
    OnyxGUI
)

# Wyszukujemy wszystkie pliki które powinny znaleźć się w folderze "Resources"
# finalnej aplikacji w formacie .app na macOS
file(GLOB_RECURSE APP_RESOURCES "${CMAKE_SOURCE_DIR}/resources/*")

# Ustawiamy właściwości zasobów aby zostały poprawnie przekopiowane do
# app bundle programu.
foreach(FILE ${APP_RESOURCES})
    # Zmieniamy ścieżkę na względną.
    file(RELATIVE_PATH NEW_FILE "${CMAKE_SOURCE_DIR}/resources" ${FILE})
    get_filename_component(NEW_FILE_PATH ${NEW_FILE} DIRECTORY)

    # Ustawiamy ścieżkę pliku w app bundle
    set_property(SOURCE ${FILE} PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${NEW_FILE_PATH}")
endforeach()

target_sources(OnyxApp PRIVATE
    main.cpp
    ${APP_RESOURCES}
)

add_dependencies(OnyxApp hdOnyx)

add_custom_command(TARGET OnyxApp PRE_BUILD
    # Tworzymy strukturę plików dla kopiowanej instalacji OpenUSD.
    # Biblioteka zostanie przekopiowana do folderu Frameworks/OpenUSD
    # z zachowaniem oryginalnej struktury wewnętrznej bibliotek i pluginów.

    # Tworzymy wymagane foldery.
    # .app/Contents/Frameworks/OpenUSD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD

    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD/lib
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD/lib/usd
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD/plugin
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD/plugin/usd

    # Kopiujemy wszystkie pliki z folderów lib/ plugin/usd oraz lib/usd.
    #
    # - lib: Zawiera biblioteki z rozszerzeniem .dylib
    COMMAND ${CMAKE_COMMAND} -E copy
        "${PXR_CMAKE_DIR}/lib/*.dylib" $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD/lib

    # - lib/usd: Zawiera pliki ładujące bibliotek z folderu lib
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PXR_CMAKE_DIR}/lib/usd $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD/lib/usd

    # - plugin/usd: Zawiera opcjonalne pluginy wraz z plikami ładującymi plugLoad.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PXR_CMAKE_DIR}/plugin/usd $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD/plugin/usd

    # Hydra Render Delegate - Onyx
    #
    # Przygotowujemy strukturę pod plugin silnika hdOnyx
    # W folderze plugin/usd -> hdOnyx/resources/
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD/plugin/usd/hdOnyx
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD/plugin/usd/hdOnyx/resources
    #
    # Kopiujemy plugin hdOnyx z poprawną strukturą oraz plikiem opisowym plugInfo.json.
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE_DIR:hdOnyx>/*.so $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD/plugin/usd

    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE_DIR:hdOnyx>/*.json $<TARGET_FILE_DIR:OnyxApp>/../Frameworks/OpenUSD/plugin/usd/hdOnyx/resources
)