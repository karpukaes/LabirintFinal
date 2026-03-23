#pragma once

// ============================================================================
// types.h
// ----------------------------------------------------------------------------
// Общие структуры и перечисления, которые используются сразу в нескольких
// модулях.
// ============================================================================

#include "config.h"
#include <cmath>

// Координата клетки.
// ВАЖНО: во всей программе используется ЕДИНАЯ система координат:
//   x - номер столбца, слева направо
//   y - номер строки, сверху вниз
//   (0, 0) - верхний левый угол поля
struct Pos {
    int x = 0;
    int y = 0;
};

inline bool operator==(const Pos& a, const Pos& b) {
    return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const Pos& a, const Pos& b) {
    return !(a == b);
}

// Направление робота для анимации и ручного режима.
enum Direction {
    DIR_UP,
    DIR_UP_RIGHT,
    DIR_RIGHT,
    DIR_DOWN_RIGHT,
    DIR_DOWN,
    DIR_DOWN_LEFT,
    DIR_LEFT,
    DIR_UP_LEFT
};

// Эвристика для A*.
enum class HeuristicType : unsigned char {
    Manhattan,
    Euclidean,
    Chebyshev,
    Zero,
    Octile
};

// Параметры визуализации.
struct AnimationSettings {
    bool visualizeSearch = true;  // показывать поэтапный обход графа
    bool animateRobot = true;     // показывать движение робота по найденному пути
    int searchDelayMs = 15;       // задержка между кадрами поиска
    int moveDelayMs = 80;         // задержка между кадрами движения по пути
};

// Параметры движения по сетке.
struct MovementSettings {
    bool allowDiagonal = true;         // разрешить диагональные переходы
    bool forbidCornerCutting = true;   // запрещать проход по диагонали через угол стены
};

// Статистика работы алгоритма.
struct SearchStats {
    bool found = false;
    int visitedCount = 0;        // сколько вершин было извлечено из очереди / PQ
    int pathLength = 0;          // длина пути в переходах (без учёта стартовой клетки)
    int pathNodeCount = 0;       // количество клеток в полном пути, включая старт и финиш
    double pathCost = 0.0;       // суммарная стоимость найденного пути
    double elapsedMs = 0.0;      // время работы алгоритма в миллисекундах

    Pos reachedExit{};           // к какому выходу пришёл алгоритм
    int reachedExitIndex = -1;   // индекс найденного выхода в массиве exits[]
};

// Проверка попадания в границы поля.
inline bool isInside(int N, int M, int x, int y) {
    return x >= 0 && x < M && y >= 0 && y < N;
}

inline bool isInside(int N, int M, Pos p) {
    return isInside(N, M, p.x, p.y);
}

// Проверка, является ли клетка выходом.
inline bool isExit(const Pos exits[MAX_EXITS], int exitCount, Pos p) {
    for (int i = 0; i < exitCount; ++i) {
        if (exits[i] == p) {
            return true;
        }
    }
    return false;
}

// Возвращает индекс выхода или -1, если клетка не является выходом.
inline int findExitIndex(const Pos exits[MAX_EXITS], int exitCount, Pos p) {
    for (int i = 0; i < exitCount; ++i) {
        if (exits[i] == p) {
            return i;
        }
    }
    return -1;
}
