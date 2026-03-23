#include "io.h"
#include "grid.h"
#include <fstream>
#include <string>

bool loadScenario(
    const char* filename,
    int& N,
    int& M,
    int field[MAX_N][MAX_M],
    Pos& start,
    Pos exits[MAX_EXITS],
    int& exitCount
) {
    std::ifstream fin(filename);
    if (!fin) {
        return false;
    }

    // Формат файла:
    // N M
    // start.x start.y
    // exitCount
    // exit1.x exit1.y
    // ...
    // GRID
    // строка_0_из_M_символов
    // ...
    if (!(fin >> N >> M)) {
        return false;
    }
    if (N <= 0 || N > MAX_N || M <= 0 || M > MAX_M) {
        return false;
    }

    if (!(fin >> start.x >> start.y)) {
        return false;
    }
    if (!isInside(N, M, start)) {
        return false;
    }

    if (!(fin >> exitCount)) {
        return false;
    }
    if (exitCount <= 0 || exitCount > MAX_EXITS) {
        return false;
    }

    for (int i = 0; i < exitCount; ++i) {
        if (!(fin >> exits[i].x >> exits[i].y)) {
            return false;
        }
        if (!isInside(N, M, exits[i])) {
            return false;
        }
    }

    std::string marker;
    if (!(fin >> marker) || marker != "GRID") {
        return false;
    }

    clearGrid(field, N, M, CELL_FREE);

    // ВАЖНО: строка y файла записывается в field[y][x] без каких-либо сдвигов,
    // перестановок и преобразований. Именно это устраняет проблему смещения
    // координат между редактором и приложением.
    for (int y = 0; y < N; ++y) {
        std::string row;
        if (!(fin >> row)) {
            return false;
        }
        if (static_cast<int>(row.size()) != M) {
            return false;
        }

        for (int x = 0; x < M; ++x) {
            const char c = row[x];
            if (c < '0' || c > '3') {
                return false;
            }
            field[y][x] = c - '0';
        }
    }

    // Старт и выходы должны стоять только на проходимых клетках.
    if (!isPassableCell(field[start.y][start.x])) {
        return false;
    }
    for (int i = 0; i < exitCount; ++i) {
        if (!isPassableCell(field[exits[i].y][exits[i].x])) {
            return false;
        }
    }

    return true;
}
