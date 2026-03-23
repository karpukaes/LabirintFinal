#include "manual.h"
#include "console_ui.h"
#include "grid.h"
#include <conio.h>

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
    const char* status = "Управление: WASD и QEZC для диагоналей, Esc - выход";

    while (true) {
        visited[robot.y][robot.x] = true;
        renderGrid(N, M, field, start, exits, exitCount, robot, dir, visited, pathMask, status);

        if (isExit(exits, exitCount, robot)) {
            renderGrid(
                N, M, field, start, exits, exitCount,
                robot, dir, visited, pathMask,
                "Выход достигнут. Нажмите Esc или Enter для возврата."
            );
            const int ch = _getch();
            if (ch == 27 || ch == 13) {
                return;
            }
        }

        const int ch = _getch();
        if (ch == 27) {
            return;
        }

        int dx = 0;
        int dy = 0;

        switch (ch) {
        case 'w': case 'W': dy = -1; break;
        case 'd': case 'D': dx =  1; break;
        case 's': case 'S': dy =  1; break;
        case 'a': case 'A': dx = -1; break;
        case 'q': case 'Q': dx = -1; dy = -1; break;
        case 'e': case 'E': dx =  1; dy = -1; break;
        case 'z': case 'Z': dx = -1; dy =  1; break;
        case 'c': case 'C': dx =  1; dy =  1; break;
        default:
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
