# Minimalna wersja wymagana do wsparcia architektury ARM64 na macOS
cmake_minimum_required (VERSION 3.19)

# Tworzymy projekt Cmake
project(Onyx VERSION 1.1)

# Wymagamy C++17 dla OpenUSD
set(CMAKE_CXX_STANDARD 17)

# Dodajemy wszystkie wymagane zewnętrzne biblioteki.
add_subdirectory(extern)

add_subdirectory(src)
