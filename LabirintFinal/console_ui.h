#pragma once

// ============================================================================
// console_ui.h
// ----------------------------------------------------------------------------
// Функции работы с консолью: очистка экрана, пауза, цветной вывод и отрисовка
// поля.
// ============================================================================

#include "types.h"

void enableANSI();
void clearScreen();
void sleepMs(int ms);
void pauseConsole();
void printTitle(const char* title);

void renderGrid(
    int N,
    int M,
    const int field[MAX_N][MAX_M],
    Pos start,
    const Pos exits[MAX_EXITS],
    int exitCount,
    Pos robot,
    Direction dir,
    const bool visited[MAX_N][MAX_M],
    const bool pathMask[MAX_N][MAX_M],
    const char* statusLine
);
