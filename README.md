# hdOnyx

## Zewnętrzne Biblioteki:
- [ImGui](https://github.com/ocornut/imgui) z backendem:
  - [GLFW](https://github.com/glfw/glfw)
  - OpenGL

## Wymagania:
- CMake 3.19+
- macOS 14
- Xcode

## Konfiguracja projektu

1. Repozytorium należy sklonować uwzględniając submoduły:
```zsh
git clone --recurse-submodules https://github.com/gracjan-j/hdOnyx.git
```
2. Konfiguracja projektu odbywa się przez CMake:
```zsh
# Wejdz do folderu sklonowanego projektu:
cd hdOnyx

# Utwórz folder `build`
mkdir build

# Wejdź do folderu `build`
cd build

# Skonfiguruj projekt przez CMake używając generatora Xcode
cmake .. -G "Xcode"

# W folderze zostanie wygenerowany projekt Xcode
# który może zostać użyty do zbudowania programu i/lub edycji plików.
```

