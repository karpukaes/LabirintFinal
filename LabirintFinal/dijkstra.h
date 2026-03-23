#pragma once
#include "types.h"

SearchStats runAutoModeDijkstra(
    int N,
    int M,
    const int field[MAX_N][MAX_M],
    Pos start,
    const Pos exits[MAX_EXITS],
    int exitCount,
    const AnimationSettings& anim,
    const MovementSettings& move
);
