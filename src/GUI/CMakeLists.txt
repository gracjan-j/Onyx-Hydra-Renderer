add_library(OnyxGUI)

target_sources(OnyxGUI PRIVATE
    src/Window.cpp
)

target_include_directories(OnyxGUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(OnyxGUI PUBLIC
    # Target ImGui uwzględnia GLFW oraz OpenGL
    ImGui
)