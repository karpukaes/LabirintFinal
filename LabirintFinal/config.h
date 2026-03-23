#pragma once

// ============================================================================
// config.h
// ----------------------------------------------------------------------------
// В этом файле собраны глобальные константы проекта и простые вспомогательные
// функции, связанные с типами клеток.
//
// Проект ориентирован на Visual Studio / Windows, поэтому подключается
// windows.h. Макрос NOMINMAX нужен для того, чтобы windows.h не подменял
// std::min и std::max своими макросами.
// ============================================================================

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

// Максимально допустимые размеры лабиринта.
const int MAX_N = 64;
const int MAX_M = 64;

// Максимально допустимое количество выходов.
const int MAX_EXITS = 16;

// Стоимость диагонального шага в евклидовой метрике.
const double DIAG_COST = 1.41421356237;

// Тип клетки в поле.
enum CellType {
    CELL_FREE = 0,
    CELL_WALL = 1,
    CELL_SAND = 2,
    CELL_WATER = 3
};

// Возвращает базовую стоимость входа в клетку.
// Стена фактически непроходима, поэтому для неё возвращается очень большое
// значение, хотя в корректной логике поиска на стену мы вообще не заходим.
inline int cellCostByCellValue(int cell) {
    switch (cell) {
    case CELL_FREE:  return 1;
    case CELL_SAND:  return 3;
    case CELL_WATER: return 5;
    default:         return 1000000000;
    }
}

// Является ли клетка проходимой.
inline bool isPassableCell(int cell) {
    return cell != CELL_WALL;
}
