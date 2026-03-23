#pragma once

// ============================================================================
// grid.h
// ----------------------------------------------------------------------------
// Функции общего назначения для работы с сеткой лабиринта.
// ============================================================================

#include "types.h"

void clearBoolMask(bool mask[MAX_N][MAX_M]);
void clearGrid(int field[MAX_N][MAX_M], int N, int M, int value = CELL_FREE);

bool canStandOnCell(int N, int M, const int field[MAX_N][MAX_M], Pos p);

// Проверяет, можно ли сделать переход из from в to с учётом:
// - границ поля
// - стен
// - разрешения / запрета диагоналей
// - запрета срезания углов
bool canMoveBetween(
    int N,
    int M,
    const int field[MAX_N][MAX_M],
    Pos from,
    Pos to,
    const MovementSettings& move
);

// Стоимость одного перехода в клетку to.
// Если переход диагональный, базовая стоимость клетки умножается на DIAG_COST.
double stepCost(const int field[MAX_N][MAX_M], Pos from, Pos to);
