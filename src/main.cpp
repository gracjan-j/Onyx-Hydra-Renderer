#define GL_SILENCE_DEPRECATION


#include <Window.h>


// Funkcja main incjalizująca GLFW oraz ImGui.
// ImGui wykonuje pętle renderowania która trwa do czasu
// manualnego zakończenia programu przez użytkownika.
int main(int argc, char *argv[])
{
    GUI::Window applicationWindow = GUI::Window();

    applicationWindow.RunRenderingLoop();

    return 0;
}
