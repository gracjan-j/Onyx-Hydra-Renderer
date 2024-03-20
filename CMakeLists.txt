# Minimalna wersja wymagana do wsparcia architektury ARM64 na macOS
cmake_minimum_required (VERSION 3.19)

# Tworzymy projekt Cmake
project(Onyx VERSION 1.1)

# Wymagamy C++17 dla OpenUSD
set(CMAKE_CXX_STANDARD 17)

# W przypadku C++17 wymagane jest włączenie wycofanych funkcji na nowych systemach macOS.
if (APPLE AND CMAKE_HOST_SYSTEM_VERSION VERSION_GREATER_EQUAL 23)
    message(STATUS "Aplikuje flagę która aktywuje wycofane funkcje z języka C++ 17 na macOS Sonoma+")
    set_directory_properties(PROPERTIES COMPILE_DEFINITIONS _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION)
endif()

# Dodajemy wszystkie wymagane zewnętrzne biblioteki.
add_subdirectory(extern)

# Dodajemy źródła programu.
# Wewnątrz folderu plików źródłowych może znajdować się plik CMakeLists.txt delegujący
# konfigurację między modułami z których składa się program.
add_subdirectory(src)
