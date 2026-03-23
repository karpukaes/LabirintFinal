#include "bfs.h"
#include "console_ui.h"
#include "grid.h"
#include "path_utils.h"
#include <queue>
#include <chrono>
#include <cstdio>

SearchStats runAutoModeBFS(
    int N,
    int M,
    const int field[MAX_N][MAX_M],
    Pos start,
    const Pos exits[MAX_EXITS],
    int exitCount,
    const AnimationSettings& anim,
    const MovementSettings& move
) {
    SearchStats stats;

    bool visited[MAX_N][MAX_M];
    bool pathMask[MAX_N][MAX_M];
    Pos parent[MAX_N][MAX_M];
    clearBoolMask(visited);
    clearBoolMask(pathMask);

    std::queue<Pos> q;
    q.push(start);
    visited[start.y][start.x] = true;
    parent[start.y][start.x] = start;

    Pos foundExit{};

    // Список допустимых направлений строится сразу в 8-связной модели.
    // Если диагонали запрещены, функция canMoveBetween сама отсечёт лишнее.
    const int dx[8] = {  1, -1,  0,  0,  1,  1, -1, -1 };
    const int dy[8] = {  0,  0,  1, -1, -1,  1, -1,  1 };

    const auto t1 = std::chrono::high_resolution_clock::now();

    while (!q.empty()) {
        const Pos cur = q.front();
        q.pop();
        ++stats.visitedCount;

        if (isExit(exits, exitCount, cur)) {
            stats.found = true;
            stats.reachedExit = cur;
            stats.reachedExitIndex = findExitIndex(exits, exitCount, cur);
            foundExit = cur;
            break;
        }

        if (anim.visualizeSearch) {
            renderGrid(
                N, M, field, start, exits, exitCount,
                cur, DIR_RIGHT, visited, pathMask,
                "BFS: выполняется поиск..."
            );
            sleepMs(anim.searchDelayMs);
        }

        for (int k = 0; k < 8; ++k) {
            const Pos next{ cur.x + dx[k], cur.y + dy[k] };
            if (!canMoveBetween(N, M, field, cur, next, move)) {
                continue;
            }
            if (visited[next.y][next.x]) {
                continue;
            }

            visited[next.y][next.x] = true;
            parent[next.y][next.x] = cur;
            q.push(next);
        }
    }

    const auto t2 = std::chrono::high_resolution_clock::now();
    stats.elapsedMs = std::chrono::duration<double, std::milli>(t2 - t1).count();

    if (!stats.found) {
        return stats;
    }

    Pos path[MAX_N * MAX_M];
    const int pathSize = buildPath(start, foundExit, parent, path);
    makePathMask(path, pathSize, pathMask);

    stats.pathNodeCount = pathSize;
    stats.pathLength = pathSize - 1;
    stats.pathCost = computePathCost(path, pathSize, field);

    if (anim.animateRobot) {
        animateRobotPath(
            N, M, field, start, exits, exitCount,
            path, pathSize, visited, pathMask, anim.moveDelayMs, "BFS"
        );
    }
    else {
        char status[160];
        std::snprintf(
            status,
            sizeof(status),
            "BFS завершён. Переходов: %d, стоимость: %.3f",
            stats.pathLength,
            stats.pathCost
        );
        renderGrid(N, M, field, start, exits, exitCount, foundExit, DIR_RIGHT, visited, pathMask, status);
    }

    return stats;
}
