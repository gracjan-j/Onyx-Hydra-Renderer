# Onyx Hydra Renderer

## Zewnętrzne Biblioteki:
- [ImGui](https://github.com/ocornut/imgui) z backendem:
  - [GLFW](https://github.com/glfw/glfw)
  - OpenGL

## Wymagania:
- CMake 3.19+
- macOS 14
- Xcode
- OpenUSD 23.08+

## Budowanie biblioteki OpenUSD
1. Należy sklonować oryginalne repozytorium OpenUSD:
```zsh
git clone https://github.com/PixarAnimationStudios/OpenUSD
```
2. Należy wywołać skrypt za pomocą interpretera języka Python
który automatyzuje proces budowania biblioteki. Projekt jest
dostosowany w sposób który zapewnia kompatybilność z OpenUSD zbudowanym
z domyślną konfiguracją skryptu.
```zsh
# W sklonowanym repozytorium OpenUSD:
python3 build_scripts/build_usd.py /Ścieżka/Instalacyjna/OpenUSD
```

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
# i ścieżki do zbudowanej biblioteki OpenUSD 23.08+
cmake .. -G "Ninja" -DUSD_PATH="/Ścieżka/Instalacyjna/OpenUSD"

# Budowanie projektu odbywa się za pomocą komendy:
cmake --build . --config Release -- -j 12

# Aplikacja .app programu znajduje się w folderze build/src/
```

