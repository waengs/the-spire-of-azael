#include "Game.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    HANDLE outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (outHandle != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(outHandle, &mode)) {
            SetConsoleMode(outHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif

    Game game;
    game.start();
    return 0;
}
