#include "grid.h"
#include <cmath>

void clearBoolMask(bool mask[MAX_N][MAX_M]) {
    for (int y = 0; y < MAX_N; ++y) {
        for (int x = 0; x < MAX_M; ++x) {
            mask[y][x] = false;
        }
    }
}

void clearGrid(int field[MAX_N][MAX_M], int N, int M, int value) {
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < M; ++x) {
            field[y][x] = value;
        }
    }
}

bool canStandOnCell(int N, int M, const int field[MAX_N][MAX_M], Pos p) {
    return isInside(N, M, p) && isPassableCell(field[p.y][p.x]);
}

bool canMoveBetween(
    int N,
    int M,
    const int field[MAX_N][MAX_M],
    Pos from,
    Pos to,
    const MovementSettings& move
) {
    // Целевая клетка должна находиться в пределах поля и не быть стеной.
    if (!canStandOnCell(N, M, field, to)) {
        return false;
    }

    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    // Допускаются только переходы в соседние клетки.
    if (std::abs(dx) > 1 || std::abs(dy) > 1 || (dx == 0 && dy == 0)) {
        return false;
    }

    const bool diagonal = (std::abs(dx) == 1 && std::abs(dy) == 1);
    if (diagonal && !move.allowDiagonal) {
        return false;
    }

    // Запрет срезания углов: если движение диагональное, то обе соседние по
    // осям клетки должны быть свободны. Иначе робот как бы "протискивается"
    // через угол стены, что обычно считается некорректным.
    if (diagonal && move.forbidCornerCutting) {
        Pos side1{ from.x + dx, from.y };
        Pos side2{ from.x, from.y + dy };

        if (!canStandOnCell(N, M, field, side1) || !canStandOnCell(N, M, field, side2)) {
            return false;
        }
    }

    return true;
}

double stepCost(const int field[MAX_N][MAX_M], Pos from, Pos to) {
    const bool diagonal = (std::abs(to.x - from.x) == 1 && std::abs(to.y - from.y) == 1);
    const double base = static_cast<double>(cellCostByCellValue(field[to.y][to.x]));
    return diagonal ? base * DIAG_COST : base;
}
