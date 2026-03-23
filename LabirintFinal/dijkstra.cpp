#include "dijkstra.h"
#include "console_ui.h"
#include "grid.h"
#include "path_utils.h"
#include <queue>
#include <chrono>
#include <cstdio>

struct DijkstraNode {
    double dist;
    Pos pos;

    bool operator<(const DijkstraNode& other) const {
        // std::priority_queue в C++ по умолчанию устроена как max-heap,
        // поэтому знак сравнения инвертируем.
        return dist > other.dist;
    }
};

SearchStats runAutoModeDijkstra(
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

    bool closed[MAX_N][MAX_M];
    bool pathMask[MAX_N][MAX_M];
    double dist[MAX_N][MAX_M];
    Pos parent[MAX_N][MAX_M];
    clearBoolMask(closed);
    clearBoolMask(pathMask);

    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < M; ++x) {
            dist[y][x] = 1e100;
        }
    }

    std::priority_queue<DijkstraNode> pq;
    dist[start.y][start.x] = 0.0;
    parent[start.y][start.x] = start;
    pq.push({ 0.0, start });

    Pos foundExit{};

    const int dx[8] = {  1, -1,  0,  0,  1,  1, -1, -1 };
    const int dy[8] = {  0,  0,  1, -1, -1,  1, -1,  1 };

    const auto t1 = std::chrono::high_resolution_clock::now();

    while (!pq.empty()) {
        const DijkstraNode currentNode = pq.top();
        pq.pop();

        const Pos cur = currentNode.pos;
        if (closed[cur.y][cur.x]) {
            continue;
        }

        closed[cur.y][cur.x] = true;
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
                cur, DIR_RIGHT, closed, pathMask,
                "Дейкстра: выполняется поиск..."
            );
            sleepMs(anim.searchDelayMs);
        }

        for (int k = 0; k < 8; ++k) {
            const Pos next{ cur.x + dx[k], cur.y + dy[k] };
            if (!canMoveBetween(N, M, field, cur, next, move)) {
                continue;
            }

            const double tentative = dist[cur.y][cur.x] + stepCost(field, cur, next);
            if (tentative < dist[next.y][next.x]) {
                dist[next.y][next.x] = tentative;
                parent[next.y][next.x] = cur;
                pq.push({ tentative, next });
            }
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
    stats.pathCost = dist[foundExit.y][foundExit.x];

    if (anim.animateRobot) {
        animateRobotPath(
            N, M, field, start, exits, exitCount,
            path, pathSize, closed, pathMask, anim.moveDelayMs, "Дейкстра"
        );
    }
    else {
        char status[160];
        std::snprintf(
            status,
            sizeof(status),
            "Дейкстра завершён. Переходов: %d, стоимость: %.3f",
            stats.pathLength,
            stats.pathCost
        );
        renderGrid(N, M, field, start, exits, exitCount, foundExit, DIR_RIGHT, closed, pathMask, status);
    }

    return stats;
}
