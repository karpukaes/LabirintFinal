#include "console_ui.h"
#include <iostream>
#include <limits>

namespace {
    const char* RESET = "\x1b[0m";
    const char* FG_WHITE = "\x1b[37m";
    const char* FG_BLACK = "\x1b[30m";
    const char* FG_GREEN = "\x1b[32m";
    const char* FG_RED = "\x1b[31m";
    const char* FG_YELLOW = "\x1b[33m";
    const char* FG_BLUE = "\x1b[34m";
    const char* FG_CYAN = "\x1b[36m";
    const char* FG_MAGENTA = "\x1b[35m";
    const char* BG_WHITE = "\x1b[47m";
    const char* BG_GREEN = "\x1b[42m";
    const char* BG_RED = "\x1b[41m";
    const char* BG_YELLOW = "\x1b[43m";
    const char* BG_BLUE = "\x1b[44m";
    const char* BG_CYAN = "\x1b[46m";
}

void enableANSI() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) {
        return;
    }

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

void clearScreen() {
    std::cout << "\x1b[2J\x1b[H";
}

void sleepMs(int ms) {
    Sleep(ms);
}

void pauseConsole() {
    std::cout << "\nНажмите Enter...";
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    std::cin.get();
}

void printTitle(const char* title) {
    std::cout << "=== " << title << " ===\n\n";
}

static char robotChar(Direction dir) {
    switch (dir) {
    case DIR_UP:         return '^';
    case DIR_UP_RIGHT:   return '/';
    case DIR_RIGHT:      return '>';
    case DIR_DOWN_RIGHT: return '\\';
    case DIR_DOWN:       return 'v';
    case DIR_DOWN_LEFT:  return '/';
    case DIR_LEFT:       return '<';
    case DIR_UP_LEFT:    return '\\';
    default:             return 'R';
    }
}

void renderGrid(
    int N,
    int M,
    const int field[MAX_N][MAX_M],
    Pos start,
    const Pos exits[MAX_EXITS],
    int exitCount,
    Pos robot,
    Direction dir,
    const bool visited[MAX_N][MAX_M],
    const bool pathMask[MAX_N][MAX_M],
    const char* statusLine
) {
    clearScreen();
    printTitle("Робот в лабиринте");

    std::cout
        << "Координаты: x - столбец, y - строка, начало в (0, 0) сверху слева\n"
        << "Обозначения: "
        << BG_GREEN  << FG_BLACK << " S " << RESET << " старт, "
        << BG_RED    << FG_WHITE << " E " << RESET << " выход, "
        << BG_WHITE  << FG_BLACK << " # " << RESET << " стена, "
        << BG_YELLOW << FG_BLACK << " . " << RESET << " песок, "
        << BG_BLUE   << FG_WHITE << " ~ " << RESET << " вода, "
        << BG_CYAN   << FG_BLACK << " * " << RESET << " путь, "
        << FG_CYAN   << " + "     << RESET << " просмотрено, "
        << FG_MAGENTA << " R "    << RESET << " робот\n\n";

    // Печатаем шкалу X сверху, чтобы визуально было проще сверять координаты
    // из HTML-редактора и из программы.
    std::cout << "    ";
    for (int x = 0; x < M; ++x) {
        std::cout << (x % 10) << "  ";
    }
    std::cout << "\n";

    for (int y = 0; y < N; ++y) {
        if (y < 10) std::cout << ' ';
        std::cout << y << " |";

        for (int x = 0; x < M; ++x) {
            const Pos p{ x, y };

            if (robot == p) {
                std::cout << BG_CYAN << FG_BLACK << ' ' << robotChar(dir) << ' ' << RESET;
                continue;
            }
            if (start == p) {
                std::cout << BG_GREEN << FG_BLACK << " S " << RESET;
                continue;
            }
            if (isExit(exits, exitCount, p)) {
                std::cout << BG_RED << FG_WHITE << " E " << RESET;
                continue;
            }
            if (field[y][x] == CELL_WALL) {
                std::cout << BG_WHITE << FG_BLACK << " # " << RESET;
                continue;
            }
            if (pathMask[y][x]) {
                std::cout << BG_CYAN << FG_BLACK << " * " << RESET;
                continue;
            }
            if (field[y][x] == CELL_SAND) {
                std::cout << BG_YELLOW << FG_BLACK << " . " << RESET;
                continue;
            }
            if (field[y][x] == CELL_WATER) {
                std::cout << BG_BLUE << FG_WHITE << " ~ " << RESET;
                continue;
            }
            if (visited[y][x]) {
                std::cout << FG_CYAN << " + " << RESET;
                continue;
            }

            std::cout << "   ";
        }
        std::cout << "| " << y << "\n";
    }

    std::cout << "\n    ";
    for (int x = 0; x < M; ++x) {
        std::cout << (x % 10) << "  ";
    }
    std::cout << "\n\n";

    if (statusLine != nullptr) {
        std::cout << statusLine << "\n";
    }
}
