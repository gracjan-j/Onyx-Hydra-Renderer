# Dodajemy moduł programu odpowiedzialny za interfejs użytkownika.
add_subdirectory(GUI)

# Dodajemy backend silnika renderowania.
add_subdirectory(OnyxRenderer)

# Dodajemy frontend (Hydra) silnika renderującego który będzie ładowany przez GUI.
add_subdirectory(hdOnyx)


# Wyszukujemy wszystkie pliki które powinny znaleźć się w folderze "Resources"
# finalnej aplikacji w formacie .app na macOS
file(GLOB_RECURSE APP_RESOURCES "${CMAKE_SOURCE_DIR}/resources/*")

# Dodajemy główny "target" pliku wykonywalnego w formacie macOS bundle
# by wymusić stworzenie pliku .app przez CMake zamiast surowego pliku binarnego.
add_executable(OnyxApp MACOSX_BUNDLE
    main.cpp
    ${APP_RESOURCES}
)

target_link_libraries(OnyxApp PRIVATE
    OnyxGUI
)

# Ustawiamy właściwości zasobów aby zostały poprawnie przekopiowane do
# app bundle programu.
foreach (FILE ${APP_RESOURCES})
    # Zmieniamy ścieżkę na względną.
    file(RELATIVE_PATH NEW_FILE "${CMAKE_SOURCE_DIR}/resources" ${FILE})
    get_filename_component(NEW_FILE_PATH ${NEW_FILE} DIRECTORY)

    # Ustawiamy ścieżkę pliku w app bundle
    set_property(SOURCE ${FILE} PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${NEW_FILE_PATH}")
endforeach ()
