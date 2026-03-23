#include "astar.h"
#include "console_ui.h"
#include "grid.h"
#include "path_utils.h"
#include <queue>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <algorithm>

static double heuristicToOneExit(Pos p, Pos exitPos, HeuristicType type) {
    const double dx = std::abs(p.x - exitPos.x);
    const double dy = std::abs(p.y - exitPos.y);

    switch (type) {
    case HeuristicType::Manhattan:
        return dx + dy;
    case HeuristicType::Euclidean:
        return std::sqrt(dx * dx + dy * dy);
    case HeuristicType::Chebyshev:
        return std::max(dx, dy);
    case HeuristicType::Zero:
        return 0.0;
    case HeuristicType::Octile: {
        const double mn = std::min(dx, dy);
        const double mx = std::max(dx, dy);
        return (mx - mn) + DIAG_COST * mn;
    }
    default:
        return dx + dy;
    }
}

static double heuristicToClosestExit(
    Pos p,
    const Pos exits[MAX_EXITS],
    int exitCount,
    HeuristicType type
) {
    double best = 1e100;
    for (int i = 0; i < exitCount; ++i) {
        const double h = heuristicToOneExit(p, exits[i], type);
        if (h < best) {
            best = h;
        }
    }
    return best;
}

struct AStarNode {
    double f;
    double g;
    Pos pos;

    bool operator<(const AStarNode& other) const {
        if (f != other.f) {
            return f > other.f;
        }
        return g > other.g;
    }
};

SearchStats runAutoModeAStar(
    int N,
    int M,
    const int field[MAX_N][MAX_M],
    Pos start,
    const Pos exits[MAX_EXITS],
    int exitCount,
    const AnimationSettings& anim,
    const MovementSettings& move,
    HeuristicType heuristicType
) {
    SearchStats stats;

    bool closed[MAX_N][MAX_M];
    bool pathMask[MAX_N][MAX_M];
    double gScore[MAX_N][MAX_M];
    Pos parent[MAX_N][MAX_M];
    clearBoolMask(closed);
    clearBoolMask(pathMask);

    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < M; ++x) {
            gScore[y][x] = 1e100;
        }
    }

    std::priority_queue<AStarNode> pq;
    gScore[start.y][start.x] = 0.0;
    parent[start.y][start.x] = start;
    pq.push({ heuristicToClosestExit(start, exits, exitCount, heuristicType), 0.0, start });

    Pos foundExit{};
    const int dx[8] = {  1, -1,  0,  0,  1,  1, -1, -1 };
    const int dy[8] = {  0,  0,  1, -1, -1,  1, -1,  1 };

    const auto t1 = std::chrono::high_resolution_clock::now();

    while (!pq.empty()) {
        const AStarNode currentNode = pq.top();
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
                "A*: выполняется поиск..."
            );
            sleepMs(anim.searchDelayMs);
        }

        for (int k = 0; k < 8; ++k) {
            const Pos next{ cur.x + dx[k], cur.y + dy[k] };
            if (!canMoveBetween(N, M, field, cur, next, move)) {
                continue;
            }

            const double tentativeG = gScore[cur.y][cur.x] + stepCost(field, cur, next);
            if (tentativeG < gScore[next.y][next.x]) {
                gScore[next.y][next.x] = tentativeG;
                parent[next.y][next.x] = cur;
                const double h = heuristicToClosestExit(next, exits, exitCount, heuristicType);
                pq.push({ tentativeG + h, tentativeG, next });
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
    stats.pathCost = gScore[foundExit.y][foundExit.x];

    if (anim.animateRobot) {
        animateRobotPath(
            N, M, field, start, exits, exitCount,
            path, pathSize, closed, pathMask, anim.moveDelayMs, "A*"
        );
    }
    else {
        char status[160];
        std::snprintf(
            status,
            sizeof(status),
            "A* завершён. Переходов: %d, стоимость: %.3f",
            stats.pathLength,
            stats.pathCost
        );
        renderGrid(N, M, field, start, exits, exitCount, foundExit, DIR_RIGHT, closed, pathMask, status);
    }

    return stats;
}
