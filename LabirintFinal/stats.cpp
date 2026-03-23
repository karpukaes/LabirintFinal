#include "stats.h"
#include "console_ui.h"
#include <iostream>
#include <fstream>
#include <iomanip>

static void printOne(const char* name, const SearchStats& s) {
    std::cout << name << "\n";
    std::cout << "  Путь найден: " << (s.found ? "да" : "нет") << "\n";
    if (s.found) {
        std::cout << "  Выход: #" << (s.reachedExitIndex + 1)
                  << " (" << s.reachedExit.x << ", " << s.reachedExit.y << ")\n";
        std::cout << "  Переходов: " << s.pathLength << "\n";
        std::cout << "  Клеток в пути: " << s.pathNodeCount << "\n";
        std::cout << "  Стоимость пути: " << std::fixed << std::setprecision(3) << s.pathCost << "\n";
    }
    std::cout << "  Посещено клеток: " << s.visitedCount << "\n";
    std::cout << "  Время: " << std::fixed << std::setprecision(3) << s.elapsedMs << " мс\n\n";
}

void showStatsScreen(const char* algorithmName, const SearchStats& stats) {
    clearScreen();
    printTitle("Статистика алгоритма");
    printOne(algorithmName, stats);
    pauseConsole();
}

void showCompareScreen(
    const SearchStats& bfsStats,
    const SearchStats& dijkstraStats,
    const SearchStats& astarStats
) {
    clearScreen();
    printTitle("Сравнение алгоритмов");
    printOne("BFS", bfsStats);
    printOne("Дейкстра", dijkstraStats);
    printOne("A*", astarStats);
}

bool saveCompareReport(
    const char* filename,
    const char* scenarioName,
    int N,
    int M,
    const SearchStats& bfsStats,
    const SearchStats& dijkstraStats,
    const SearchStats& astarStats
) {
    std::ofstream fout(filename);
    if (!fout) {
        return false;
    }

    fout << "СРАВНЕНИЕ АЛГОРИТМОВ ПОИСКА ПУТИ\n";
    fout << "Сценарий: " << scenarioName << "\n";
    fout << "Размер поля: " << N << " x " << M << "\n\n";

    auto writeOne = [&](const char* name, const SearchStats& s) {
        fout << name << "\n";
        fout << "  Путь найден: " << (s.found ? "да" : "нет") << "\n";
        if (s.found) {
            fout << "  Выход: #" << (s.reachedExitIndex + 1)
                 << " (" << s.reachedExit.x << ", " << s.reachedExit.y << ")\n";
            fout << "  Переходов: " << s.pathLength << "\n";
            fout << "  Клеток в пути: " << s.pathNodeCount << "\n";
            fout << "  Стоимость пути: " << std::fixed << std::setprecision(3) << s.pathCost << "\n";
        }
        fout << "  Посещено клеток: " << s.visitedCount << "\n";
        fout << "  Время: " << std::fixed << std::setprecision(3) << s.elapsedMs << " мс\n\n";
    };

    writeOne("BFS", bfsStats);
    writeOne("Дейкстра", dijkstraStats);
    writeOne("A*", astarStats);
    return true;
}
