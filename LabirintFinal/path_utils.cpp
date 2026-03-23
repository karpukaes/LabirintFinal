#include "path_utils.h"
#include "console_ui.h"
#include "grid.h"
#include <cstdio>
#include <cmath>

int buildPath(
    Pos start,
    Pos finish,
    const Pos parent[MAX_N][MAX_M],
    Pos path[MAX_N * MAX_M]
) {
    // Идём от финиша к старту по массиву parent, затем разворачиваем путь.
    int pathSize = 0;
    Pos cur = finish;

    while (true) {
        path[pathSize++] = cur;
        if (cur == start) {
            break;
        }
        cur = parent[cur.y][cur.x];
    }

    for (int i = 0; i < pathSize / 2; ++i) {
        const Pos temp = path[i];
        path[i] = path[pathSize - 1 - i];
        path[pathSize - 1 - i] = temp;
    }

    return pathSize;
}

void makePathMask(
    const Pos path[MAX_N * MAX_M],
    int pathSize,
    bool pathMask[MAX_N][MAX_M]
) {
    clearBoolMask(pathMask);
    for (int i = 0; i < pathSize; ++i) {
        pathMask[path[i].y][path[i].x] = true;
    }
}

Direction directionBetween(Pos a, Pos b) {
    const int dx = b.x - a.x;
    const int dy = b.y - a.y;

    if (dx == 0 && dy < 0) return DIR_UP;
    if (dx > 0 && dy < 0)  return DIR_UP_RIGHT;
    if (dx > 0 && dy == 0) return DIR_RIGHT;
    if (dx > 0 && dy > 0)  return DIR_DOWN_RIGHT;
    if (dx == 0 && dy > 0) return DIR_DOWN;
    if (dx < 0 && dy > 0)  return DIR_DOWN_LEFT;
    if (dx < 0 && dy == 0) return DIR_LEFT;
    return DIR_UP_LEFT;
}

double computePathCost(
    const Pos path[MAX_N * MAX_M],
    int pathSize,
    const int field[MAX_N][MAX_M]
) {
    double total = 0.0;
    for (int i = 1; i < pathSize; ++i) {
        total += stepCost(field, path[i - 1], path[i]);
    }
    return total;
}

void animateRobotPath(
    int N,
    int M,
    const int field[MAX_N][MAX_M],
    Pos start,
    const Pos exits[MAX_EXITS],
    int exitCount,
    const Pos path[MAX_N * MAX_M],
    int pathSize,
    const bool visited[MAX_N][MAX_M],
    const bool pathMask[MAX_N][MAX_M],
    int moveDelayMs,
    const char* titlePrefix
) {
    if (pathSize <= 0) {
        return;
    }

    char status[160];
    for (int i = 0; i < pathSize; ++i) {
        Direction dir = DIR_RIGHT;
        if (i + 1 < pathSize) {
            dir = directionBetween(path[i], path[i + 1]);
        }

        std::snprintf(
            status,
            sizeof(status),
            "%s: шаг анимации %d из %d",
            titlePrefix,
            i,
            pathSize - 1
        );

        renderGrid(
            N,
            M,
            field,
            start,
            exits,
            exitCount,
            path[i],
            dir,
            visited,
            pathMask,
            status
        );

        sleepMs(moveDelayMs);
    }
}
