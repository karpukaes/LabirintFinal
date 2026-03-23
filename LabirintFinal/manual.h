#pragma once
#include "types.h"

void runManualMode(
    int N,
    int M,
    const int field[MAX_N][MAX_M],
    Pos start,
    const Pos exits[MAX_EXITS],
    int exitCount,
    const MovementSettings& move
);
