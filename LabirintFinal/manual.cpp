#include "manual.h"
#include "console_ui.h"
#include "grid.h"
#include <conio.h>
#include <cwctype>

static Direction dirFromDelta(int dx, int dy) {
    if (dx == 0 && dy < 0) return DIR_UP;
    if (dx > 0 && dy < 0)  return DIR_UP_RIGHT;
    if (dx > 0 && dy == 0) return DIR_RIGHT;
    if (dx > 0 && dy > 0)  return DIR_DOWN_RIGHT;
    if (dx == 0 && dy > 0) return DIR_DOWN;
    if (dx < 0 && dy > 0)  return DIR_DOWN_LEFT;
    if (dx < 0 && dy == 0) return DIR_LEFT;
    return DIR_UP_LEFT;
}

// Преобразование нажатой клавиши в вектор смещения.
// Используем _getwch(), а не _getch(), чтобы корректно получать не только
// латинские, но и русские буквы в Unicode. Это позволяет управлять роботом
// одинаково удобно как в английской, так и в русской раскладке.
static bool tryGetMoveDelta(wint_t key, int& dx, int& dy) {
    dx = 0;
    dy = 0;

    const wint_t ch = std::towlower(key);

    // Английская раскладка:
    //   q w e
    //   a   d
    //   z s c
    // Русская раскладка на тех же физических клавишах:
    //   й ц у
    //   ф   в
    //   я ы с
    switch (ch) {
    case L'w': case L'ц': dy = -1; return true;
    case L'd': case L'в': dx = 1; return true;
    case L's': case L'ы': dy = 1; return true;
    case L'a': case L'ф': dx = -1; return true;
    case L'q': case L'й': dx = -1; dy = -1; return true;
    case L'e': case L'у': dx = 1; dy = -1; return true;
    case L'z': case L'я': dx = -1; dy = 1; return true;
    case L'c': case L'с': dx = 1; dy = 1; return true;
    default:
        return false;
    }
}

void runManualMode(
    int N,
    int M,
    const int field[MAX_N][MAX_M],
    Pos start,
    const Pos exits[MAX_EXITS],
    int exitCount,
    const MovementSettings& move
) {
    bool visited[MAX_N][MAX_M];
    bool pathMask[MAX_N][MAX_M];
    clearBoolMask(visited);
    clearBoolMask(pathMask);

    Pos robot = start;
    Direction dir = DIR_RIGHT;
    const char* status = "Управление: WASD/QEZC или ФЫВЦ/ЙУЯС, Esc - выход";

    while (true) {
        visited[robot.y][robot.x] = true;
        renderGrid(N, M, field, start, exits, exitCount, robot, dir, visited, pathMask, status);

        if (isExit(exits, exitCount, robot)) {
            renderGrid(
                N, M, field, start, exits, exitCount,
                robot, dir, visited, pathMask,
                "Выход достигнут. Нажмите Esc или Enter для возврата."
            );
            const wint_t ch = _getwch();
            if (ch == 27 || ch == 13) {
                return;
            }
        }

        const wint_t ch = _getwch();
        if (ch == 27) {
            return;
        }

        int dx = 0;
        int dy = 0;

        if (!tryGetMoveDelta(ch, dx, dy)) {
            status = "Неизвестная команда.";
            continue;
        }

        const Pos next{ robot.x + dx, robot.y + dy };
        if (canMoveBetween(N, M, field, robot, next, move)) {
            robot = next;
            dir = dirFromDelta(dx, dy);
            status = "Ход выполнен.";
        }
        else {
            status = "Ход запрещён: стена, граница, диагонали отключены или срезается угол.";
        }
    }
}
