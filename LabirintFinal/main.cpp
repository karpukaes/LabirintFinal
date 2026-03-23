#include <iostream>
#include <limits>
#include <locale>
#include <string>
#include "console_ui.h"
#include "io.h"
#include "manual.h"
#include "bfs.h"
#include "dijkstra.h"
#include "astar.h"
#include "stats.h"

static AnimationSettings askAnimationSettings() {
    AnimationSettings a;

    std::cout << "Выберите скорость анимации:\n";
    std::cout << "1 - Медленно\n";
    std::cout << "2 - Нормально\n";
    std::cout << "3 - Быстро\n";
    std::cout << "4 - Только итоговый путь\n";
    std::cout << "5 - Без анимации\n";
    std::cout << "Ваш выбор: ";

    int mode = 2;
    std::cin >> mode;

    switch (mode) {
    case 1:
        a.visualizeSearch = true;
        a.animateRobot = true;
        a.searchDelayMs = 60;
        a.moveDelayMs = 180;
        break;
    case 2:
        a.visualizeSearch = true;
        a.animateRobot = true;
        a.searchDelayMs = 20;
        a.moveDelayMs = 90;
        break;
    case 3:
        a.visualizeSearch = true;
        a.animateRobot = true;
        a.searchDelayMs = 3;
        a.moveDelayMs = 25;
        break;
    case 4:
        a.visualizeSearch = false;
        a.animateRobot = true;
        a.searchDelayMs = 0;
        a.moveDelayMs = 70;
        break;
    case 5:
        a.visualizeSearch = false;
        a.animateRobot = false;
        a.searchDelayMs = 0;
        a.moveDelayMs = 0;
        break;
    default:
        break;
    }

    return a;
}

static HeuristicType askHeuristicType() {
    std::cout << "Выберите эвристику для A*:\n";
    std::cout << "1 - Manhattan\n";
    std::cout << "2 - Euclidean\n";
    std::cout << "3 - Chebyshev\n";
    std::cout << "4 - Zero (эквивалент Дейкстре)\n";
    std::cout << "5 - Octile\n";
    std::cout << "Ваш выбор: ";

    int choice = 1;
    std::cin >> choice;

    switch (choice) {
    case 2: return HeuristicType::Euclidean;
    case 3: return HeuristicType::Chebyshev;
    case 4: return HeuristicType::Zero;
    case 5: return HeuristicType::Octile;
    default: return HeuristicType::Manhattan;
    }
}

static const char* heuristicName(HeuristicType type) {
    switch (type) {
    case HeuristicType::Manhattan: return "Manhattan";
    case HeuristicType::Euclidean: return "Euclidean";
    case HeuristicType::Chebyshev: return "Chebyshev";
    case HeuristicType::Zero:      return "Zero";
    case HeuristicType::Octile:    return "Octile";
    default:                       return "Unknown";
    }
}

static void showLoadedScenarioInfo(int N, int M, Pos start, const Pos exits[MAX_EXITS], int exitCount) {
    std::cout << "\nСценарий загружен успешно.\n";
    std::cout << "Размер: " << N << " x " << M << "\n";
    std::cout << "Координаты: x - столбец, y - строка, начало в (0, 0)\n";
    std::cout << "Старт: (" << start.x << ", " << start.y << ")\n";
    std::cout << "Выходов: " << exitCount << "\n";
    for (int i = 0; i < exitCount; ++i) {
        std::cout << "  Выход " << (i + 1) << ": (" << exits[i].x << ", " << exits[i].y << ")\n";
    }
    std::cout << '\n';
}

int main() {
    enableANSI();
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    setlocale(LC_ALL, ".UTF-8");
    setlocale(LC_ALL, "RU");

    int N = 0;
    int M = 0;
    int field[MAX_N][MAX_M]{};
    Pos start{};
    Pos exits[MAX_EXITS]{};
    int exitCount = 0;

    MovementSettings moveSettings{};
    HeuristicType heuristicType = HeuristicType::Manhattan;
    std::string scenarioPath;

    while (true) {
        std::cout << "=== Проект: Робот в лабиринте — объединённая версия Stage 4 + Stage 6 ===\n";
        std::cout << "1 - Загрузить сценарий из файла\n";
        std::cout << "0 - Выход\n";
        std::cout << "Ваш выбор: ";

        int mainChoice = -1;
        std::cin >> mainChoice;
        if (!std::cin) {
            std::cin.clear();
            std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
            continue;
        }

        if (mainChoice == 0) {
            break;
        }
        if (mainChoice != 1) {
            continue;
        }

        std::cout << "Введите путь к файлу сценария: ";
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        std::getline(std::cin, scenarioPath);

        if (!loadScenario(scenarioPath.c_str(), N, M, field, start, exits, exitCount)) {
            std::cout << "Ошибка: не удалось загрузить сценарий.\n\n";
            continue;
        }

        showLoadedScenarioInfo(N, M, start, exits, exitCount);

        bool backToMain = false;
        while (!backToMain) {
            std::cout << "Текущие настройки:\n";
            std::cout << "  Диагонали: " << (moveSettings.allowDiagonal ? "включены" : "выключены") << "\n";
            std::cout << "  Запрет срезания углов: " << (moveSettings.forbidCornerCutting ? "да" : "нет") << "\n";
            std::cout << "  Эвристика A*: " << heuristicName(heuristicType) << "\n\n";

            std::cout << "Выберите режим:\n";
            std::cout << "1 - Ручной режим\n";
            std::cout << "2 - BFS\n";
            std::cout << "3 - Дейкстра\n";
            std::cout << "4 - A*\n";
            std::cout << "5 - Сравнение BFS / Дейкстра / A*\n";
            std::cout << "6 - Настройки перемещения\n";
            std::cout << "7 - Выбор эвристики A*\n";
            std::cout << "0 - Назад\n";
            std::cout << "Ваш выбор: ";

            int mode = -1;
            std::cin >> mode;
            if (!std::cin) {
                std::cin.clear();
                std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
                continue;
            }

            if (mode == 0) {
                backToMain = true;
                std::cout << '\n';
                continue;
            }

            if (mode == 6) {
                int value = 0;
                std::cout << "Разрешить диагональные ходы? (1 - да, 0 - нет): ";
                std::cin >> value;
                moveSettings.allowDiagonal = (value != 0);

                std::cout << "Запретить срезание углов? (1 - да, 0 - нет): ";
                std::cin >> value;
                moveSettings.forbidCornerCutting = (value != 0);
                std::cout << '\n';
                continue;
            }

            if (mode == 7) {
                heuristicType = askHeuristicType();
                std::cout << '\n';
                continue;
            }

            if (mode == 1) {
                runManualMode(N, M, field, start, exits, exitCount, moveSettings);
                continue;
            }

            const AnimationSettings anim = askAnimationSettings();

            if (mode == 2) {
                const SearchStats bfsStats = runAutoModeBFS(N, M, field, start, exits, exitCount, anim, moveSettings);
                showStatsScreen("BFS", bfsStats);
            }
            else if (mode == 3) {
                const SearchStats dStats = runAutoModeDijkstra(N, M, field, start, exits, exitCount, anim, moveSettings);
                showStatsScreen("Дейкстра", dStats);
            }
            else if (mode == 4) {
                const SearchStats aStats = runAutoModeAStar(N, M, field, start, exits, exitCount, anim, moveSettings, heuristicType);
                showStatsScreen("A*", aStats);
            }
            else if (mode == 5) {
                const SearchStats bfsStats = runAutoModeBFS(N, M, field, start, exits, exitCount, anim, moveSettings);
                const SearchStats dStats = runAutoModeDijkstra(N, M, field, start, exits, exitCount, anim, moveSettings);
                const SearchStats aStats = runAutoModeAStar(N, M, field, start, exits, exitCount, anim, moveSettings, heuristicType);

                showCompareScreen(bfsStats, dStats, aStats);

                std::cout << "Сохранить отчёт в файл? (1 - да, 0 - нет): ";
                int save = 0;
                std::cin >> save;
                if (save == 1) {
                    std::cout << "Введите имя файла отчёта: ";
                    std::string reportPath;
                    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
                    std::getline(std::cin, reportPath);

                    if (saveCompareReport(reportPath.c_str(), scenarioPath.c_str(), N, M, bfsStats, dStats, aStats)) {
                        std::cout << "Отчёт сохранён.\n";
                    }
                    else {
                        std::cout << "Ошибка сохранения отчёта.\n";
                    }
                }
                pauseConsole();
            }
        }
    }

    return 0;
}
