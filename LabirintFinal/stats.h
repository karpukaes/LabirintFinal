#pragma once
#include "types.h"

void showStatsScreen(const char* algorithmName, const SearchStats& stats);
void showCompareScreen(
    const SearchStats& bfsStats,
    const SearchStats& dijkstraStats,
    const SearchStats& astarStats
);

bool saveCompareReport(
    const char* filename,
    const char* scenarioName,
    int N,
    int M,
    const SearchStats& bfsStats,
    const SearchStats& dijkstraStats,
    const SearchStats& astarStats
);
