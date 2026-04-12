#include "uart_game.h"

#include "console_ui.h"
#include "grid.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

    constexpr int UART_STEP_DELAY_MS = 180;

    // Внутренний тип принятой команды протокола.
    enum class UartCommandType {
        None,
        Move,
        Ack,
        Status,
        Reset,
        Help,
        Exit
    };

    // Подробная классификация ошибок разбора пакета.
    enum class ParseErrorType {
        None,
        EmptyPacket,
        MissingDollar,
        MissingStar,
        EmptyType,
        UnsupportedType,
        FieldWithoutEquals,
        UnknownField,
        MissingSeq,
        EmptySeq,
        DuplicateSeq,
        TrailingAfterStar
    };

    // Тип ошибки выполнения команды движения.
    enum class RuntimeErrorType {
        None,
        InvalidStepCommand,
        OutOfBounds,
        Wall,
        CornerCutting,
        DiagonalDisabled,
        AckRequired
    };

    struct ParsedPacket {
        bool ok = false;
        UartCommandType type = UartCommandType::None;
        std::string sequence;
        ParseErrorType parseError = ParseErrorType::None;
        std::size_t errorPos = 0;
        std::string errorText;
    };

    struct RuntimeErrorInfo {
        RuntimeErrorType type = RuntimeErrorType::None;
        std::size_t stepIndex = 0; // индекс внутри SEQ, начиная с 0
        char stepCommand = '?';
        Pos position{};            // позиция робота, на которой произошла ошибка
        std::string text;
    };

    // Состояние всей сессии ручного управления через протокол.
    struct SessionState {
        Pos robot{};
        Direction dir = DIR_RIGHT;
        bool visited[MAX_N][MAX_M]{};
        bool trail[MAX_N][MAX_M]{};
        std::string executedPath;  // только успешно выполненные шаги
        int stepCount = 0;
        int errorCount = 0;
        double pathCost = 0.0;
        bool ackRequired = false;
        bool showErrorBanner = false;
        std::string screenErrorText;
        RuntimeErrorInfo lastError{};
    };

    struct SessionLogger {
        std::ofstream stream;
        std::string filePath;
    };

    static std::string makeTimestampForFileName() {
        const std::time_t now = std::time(nullptr);
        std::tm tmNow{};
#ifdef _WIN32
        localtime_s(&tmNow, &now);
#else
        tmNow = *std::localtime(&now);
#endif
        char buffer[64]{};
        std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tmNow);
        return std::string(buffer);
    }

    static std::string makeTimestampForLogLine() {
        const std::time_t now = std::time(nullptr);
        std::tm tmNow{};
#ifdef _WIN32
        localtime_s(&tmNow, &now);
#else
        tmNow = *std::localtime(&now);
#endif
        char buffer[64]{};
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmNow);
        return std::string(buffer);
    }

    static SessionLogger openSessionLogger(const char* scenarioPath) {
        SessionLogger logger;
        logger.filePath = "uart_session_log_" + makeTimestampForFileName() + ".txt";
        logger.stream.open(logger.filePath, std::ios::out | std::ios::trunc);
        if (logger.stream) {
            logger.stream << "UART session log\n";
            logger.stream << "Scenario=" << (scenarioPath ? scenarioPath : "") << "\n";
            logger.stream << "StartedAt=" << makeTimestampForLogLine() << "\n";
            logger.stream << "----------------------------------------\n";
        }
        return logger;
    }

    static void logPacket(SessionLogger& logger, const char* direction, const std::string& packet) {
        if (!logger.stream) return;
        logger.stream << "[" << makeTimestampForLogLine() << "] " << direction << " " << packet << '\n';
        logger.stream.flush();
    }

    static void logSummary(SessionLogger& logger, const SessionState& s, bool goalReached) {
        if (!logger.stream) return;
        logger.stream << "----------------------------------------\n";
        logger.stream << "FinishedAt=" << makeTimestampForLogLine() << "\n";
        logger.stream << "GoalReached=" << (goalReached ? 1 : 0) << "\n";
        logger.stream << "Steps=" << s.stepCount << "\n";
        logger.stream << "Errors=" << s.errorCount << "\n";
        logger.stream << "Cost=" << s.pathCost << "\n";
        logger.stream << "Path=" << s.executedPath << "\n";
        logger.stream.flush();
    }

    static std::string toUpperAscii(std::string s) {
        for (char& ch : s) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        return s;
    }

    static const char* cellName(int cell, bool isGoal) {
        if (isGoal) return "EXIT";
        switch (cell) {
        case CELL_FREE:  return "FREE";
        case CELL_SAND:  return "SAND";
        case CELL_WATER: return "WATER";
        case CELL_WALL:  return "WALL";
        default:         return "UNKNOWN";
        }
    }

    static const char* parseErrorCode(ParseErrorType type) {
        switch (type) {
        case ParseErrorType::EmptyPacket:      return "EMPTY_PACKET";
        case ParseErrorType::MissingDollar:    return "MISSING_DOLLAR";
        case ParseErrorType::MissingStar:      return "MISSING_STAR";
        case ParseErrorType::EmptyType:        return "EMPTY_TYPE";
        case ParseErrorType::UnsupportedType:  return "UNSUPPORTED_TYPE";
        case ParseErrorType::FieldWithoutEquals:return "FIELD_WITHOUT_EQUALS";
        case ParseErrorType::UnknownField:     return "UNKNOWN_FIELD";
        case ParseErrorType::MissingSeq:       return "MISSING_SEQ";
        case ParseErrorType::EmptySeq:         return "EMPTY_SEQ";
        case ParseErrorType::DuplicateSeq:     return "DUPLICATE_SEQ";
        case ParseErrorType::TrailingAfterStar:return "TRAILING_AFTER_STAR";
        default:                               return "NONE";
        }
    }

    static const char* runtimeErrorCode(RuntimeErrorType type) {
        switch (type) {
        case RuntimeErrorType::InvalidStepCommand: return "INVALID_STEP_COMMAND";
        case RuntimeErrorType::OutOfBounds:        return "OUT_OF_BOUNDS";
        case RuntimeErrorType::Wall:               return "WALL";
        case RuntimeErrorType::CornerCutting:      return "CORNER_CUTTING";
        case RuntimeErrorType::DiagonalDisabled:   return "DIAGONAL_DISABLED";
        case RuntimeErrorType::AckRequired:        return "ACK_REQUIRED";
        default:                                   return "NONE";
        }
    }

    static Direction dirFromDelta(int dx, int dy) {
        if (dx == 0 && dy < 0) return DIR_UP;
        if (dx > 0 && dy < 0)  return DIR_UP_RIGHT;
        if (dx > 0 && dy == 0) return DIR_RIGHT;
        if (dx > 0 && dy > 0)  return DIR_DOWN_RIGHT;
        if (dx == 0 && dy > 0) return DIR_DOWN;
        if (dx < 0 && dy > 0)  return DIR_DOWN_LEFT;
        if (dx < 0 && dy == 0) return DIR_LEFT;
        return DIR_UP_LEFT;
    }

    static bool decodeStepCommand(char cmd, int& dx, int& dy) {
        dx = 0;
        dy = 0;
        switch (cmd) {
        case 'W': dy = -1; return true;
        case 'D': dx = 1; return true;
        case 'S': dy = 1; return true;
        case 'A': dx = -1; return true;
        case 'Q': dx = -1; dy = -1; return true;
        case 'E': dx = 1; dy = -1; return true;
        case 'Z': dx = -1; dy = 1; return true;
        case 'C': dx = 1; dy = 1; return true;
        default: return false;
        }
    }

    static std::string makeStatusLine(const SessionState& s, int cell, bool isGoal) {
        std::ostringstream out;
        out << "UART-сеанс. Координаты робота: (" << s.robot.x << ", " << s.robot.y << ")"
            << ". Шагов: " << s.stepCount
            << ". Ошибок: " << s.errorCount
            << ". Стоимость: " << s.pathCost
            << ". Клетка: " << cellName(cell, isGoal);
        if (s.ackRequired) {
            out << " Ожидается подтверждение ошибки CMD_ACK.";
        }
        return out.str();
    }

    static std::string makeParseErrorBannerText(const ParsedPacket& p) {
        std::ostringstream out;
        out << "Ошибка пакета: " << parseErrorCode(p.parseError)
            << ". " << p.errorText
            << ". Позиция в строке: " << p.errorPos << ".";
        return out.str();
    }

    static std::string makeRuntimeErrorBannerText(const RuntimeErrorInfo& e) {
        std::ostringstream out;
        out << "Ошибка выполнения: " << runtimeErrorCode(e.type)
            << ". " << e.text;
        if (e.stepCommand != '?') {
            out << ". Команда: " << e.stepCommand;
        }
        out << ". Индекс шага: " << e.stepIndex << ".";
        return out.str();
    }

    static void redraw(
        int N,
        int M,
        const int field[MAX_N][MAX_M],
        Pos start,
        const Pos exits[MAX_EXITS],
        int exitCount,
        const SessionState& state,
        const std::string& statusLine
    ) {
        renderGrid(
            N, M, field, start, exits, exitCount,
            state.robot, state.dir, state.visited, state.trail, statusLine.c_str()
        );

        if (state.showErrorBanner) {
            std::cout << "\n=== ОШИБКА ===\n";
            std::cout << state.screenErrorText << "\n";
            std::cout << "Требуется команда: $CMD_ACK*\n";
            std::cout << "================\n";
        }
    }

    static void emitPacket(SessionLogger& logger, const std::string& packet) {
        std::cout << packet << "\n";
        logPacket(logger, "TX", packet);
    }

    static void emitPackets(SessionLogger& logger, const std::vector<std::string>& packets) {
        for (const std::string& packet : packets) {
            emitPacket(logger, packet);
        }
    }

    static std::string buildReadyPacket(const SessionState& s, const Pos exits[MAX_EXITS], int exitCount, const int field[MAX_N][MAX_M]) {
        const bool goal = isExit(exits, exitCount, s.robot);
        std::ostringstream out;
        out << "$STATE,STATUS=READY,X=" << s.robot.x
            << ",Y=" << s.robot.y
            << ",CELL=" << cellName(field[s.robot.y][s.robot.x], goal)
            << ",STEPS=" << s.stepCount
            << ",ERRORS=" << s.errorCount
            << ",COST=" << s.pathCost
            << ",ACK=" << (s.ackRequired ? 1 : 0)
            << "*";
        return out.str();
    }

    static std::vector<std::string> buildHelpPackets() {
        return {
            "$INFO,STATUS=HELP,CMDS=CMD_MOVE|CMD_ACK|CMD_STATUS|CMD_RESET|CMD_HELP|CMD_EXIT*",
            "$INFO,MOVE_FORMAT=$CMD_MOVE,SEQ=WASDQEZC*",
            "$INFO,EXIT_FORMAT=$CMD_EXIT*",
            "$INFO,NOTE=QEZC_DIAGONALS_AND_ACK_REQUIRED_AFTER_ERROR*"
        };
    }

    static std::string buildStatePacket(const SessionState& s, const Pos exits[MAX_EXITS], int exitCount, const int field[MAX_N][MAX_M], const char* status, char lastCmd) {
        const bool goal = isExit(exits, exitCount, s.robot);
        std::ostringstream out;
        out << "$STATE,STATUS=" << status
            << ",X=" << s.robot.x
            << ",Y=" << s.robot.y
            << ",CELL=" << cellName(field[s.robot.y][s.robot.x], goal)
            << ",STEPS=" << s.stepCount
            << ",ERRORS=" << s.errorCount
            << ",COST=" << s.pathCost
            << ",LAST=" << lastCmd
            << ",ACK=" << (s.ackRequired ? 1 : 0)
            << "*";
        return out.str();
    }

    static std::string buildDetailPacket(const SessionState& s) {
        std::ostringstream out;
        out << "$DETAIL,PATH=" << s.executedPath
            << ",PATH_LEN=" << s.executedPath.size()
            << ",STEPS=" << s.stepCount
            << ",ERRORS=" << s.errorCount
            << ",COST=" << s.pathCost
            << "*";
        return out.str();
    }

    static std::string buildParseErrorPacket(const ParsedPacket& p, int errorCount) {
        std::ostringstream out;
        out << "$ERROR,STAGE=PARSE,TYPE=" << parseErrorCode(p.parseError)
            << ",POS=" << p.errorPos
            << ",ERRORS=" << errorCount
            << ",TEXT=" << p.errorText
            << ",NEED_ACK=1*";
        return out.str();
    }

    static std::string buildRuntimeErrorPacket(const RuntimeErrorInfo& e, int errorCount) {
        std::ostringstream out;
        out << "$ERROR,STAGE=EXEC,TYPE=" << runtimeErrorCode(e.type)
            << ",STEP_INDEX=" << e.stepIndex
            << ",CMD=" << e.stepCommand
            << ",X=" << e.position.x
            << ",Y=" << e.position.y
            << ",ERRORS=" << errorCount
            << ",TEXT=" << e.text
            << ",NEED_ACK=1*";
        return out.str();
    }

    static std::string buildAckOkPacket(const SessionState& s) {
        std::ostringstream out;
        out << "$STATE,STATUS=ACK_OK,X=" << s.robot.x
            << ",Y=" << s.robot.y
            << ",STEPS=" << s.stepCount
            << ",ERRORS=" << s.errorCount
            << ",COST=" << s.pathCost
            << ",ACK=0*";
        return out.str();
    }

    static std::vector<std::string> buildGoalPackets(const SessionState& s) {
        std::ostringstream result;
        result << "$RESULT,STATUS=GOAL_REACHED,X=" << s.robot.x
            << ",Y=" << s.robot.y
            << ",STEPS=" << s.stepCount
            << ",ERRORS=" << s.errorCount
            << ",COST=" << s.pathCost
            << ",PATH_LEN=" << s.executedPath.size()
            << "*";
        return { result.str(), buildDetailPacket(s) };
    }

    static ParsedPacket parsePacket(std::string line) {
        ParsedPacket result;

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            result.parseError = ParseErrorType::EmptyPacket;
            result.errorText = "EMPTY_LINE";
            result.errorPos = 0;
            return result;
        }

        if (line.front() != '$') {
            result.parseError = ParseErrorType::MissingDollar;
            result.errorText = "PACKET_MUST_START_WITH_DOLLAR";
            result.errorPos = 0;
            return result;
        }

        const std::size_t starPos = line.find('*');
        if (starPos == std::string::npos) {
            result.parseError = ParseErrorType::MissingStar;
            result.errorText = "PACKET_MUST_CONTAIN_STAR";
            result.errorPos = line.size();
            return result;
        }

        if (starPos + 1 != line.size()) {
            result.parseError = ParseErrorType::TrailingAfterStar;
            result.errorText = "ONLY_ONE_PACKET_PER_LINE";
            result.errorPos = starPos + 1;
            return result;
        }

        const std::string payload = line.substr(1, starPos - 1);
        if (payload.empty()) {
            result.parseError = ParseErrorType::EmptyType;
            result.errorText = "TYPE_IS_EMPTY";
            result.errorPos = 1;
            return result;
        }

        const std::size_t commaPos = payload.find(',');
        const std::string type = toUpperAscii(payload.substr(0, commaPos));
        if (type.empty()) {
            result.parseError = ParseErrorType::EmptyType;
            result.errorText = "TYPE_IS_EMPTY";
            result.errorPos = 1;
            return result;
        }

        if (type == "CMD_ACK") {
            result.ok = true;
            result.type = UartCommandType::Ack;
            return result;
        }
        if (type == "CMD_STATUS") {
            result.ok = true;
            result.type = UartCommandType::Status;
            return result;
        }
        if (type == "CMD_RESET") {
            result.ok = true;
            result.type = UartCommandType::Reset;
            return result;
        }
        if (type == "CMD_HELP") {
            result.ok = true;
            result.type = UartCommandType::Help;
            return result;
        }
        if (type != "CMD_MOVE") {
            result.parseError = ParseErrorType::UnsupportedType;
            result.errorText = "UNKNOWN_PACKET_TYPE";
            result.errorPos = 1;
            return result;
        }

        result.type = UartCommandType::Move;
        if (commaPos == std::string::npos) {
            result.parseError = ParseErrorType::MissingSeq;
            result.errorText = "CMD_MOVE_REQUIRES_SEQ";
            result.errorPos = payload.size();
            return result;
        }

        bool seqFound = false;
        std::size_t fieldStart = commaPos + 1;
        while (fieldStart < payload.size()) {
            const std::size_t nextComma = payload.find(',', fieldStart);
            const std::size_t fieldEnd = (nextComma == std::string::npos) ? payload.size() : nextComma;
            const std::string field = payload.substr(fieldStart, fieldEnd - fieldStart);

            const std::size_t eqPos = field.find('=');
            if (eqPos == std::string::npos) {
                result.parseError = ParseErrorType::FieldWithoutEquals;
                result.errorText = "FIELD_WITHOUT_EQUALS";
                result.errorPos = 1 + fieldStart;
                return result;
            }

            const std::string key = toUpperAscii(field.substr(0, eqPos));
            const std::string value = toUpperAscii(field.substr(eqPos + 1));

            if (key == "SEQ") {
                if (seqFound) {
                    result.parseError = ParseErrorType::DuplicateSeq;
                    result.errorText = "DUPLICATE_SEQ_FIELD";
                    result.errorPos = 1 + fieldStart;
                    return result;
                }
                seqFound = true;
                if (value.empty()) {
                    result.parseError = ParseErrorType::EmptySeq;
                    result.errorText = "SEQ_VALUE_IS_EMPTY";
                    result.errorPos = 1 + fieldStart + eqPos + 1;
                    return result;
                }
                result.sequence = value;
            }
            else {
                result.parseError = ParseErrorType::UnknownField;
                result.errorText = "UNKNOWN_FIELD_NAME";
                result.errorPos = 1 + fieldStart;
                return result;
            }

            if (nextComma == std::string::npos) {
                break;
            }
            fieldStart = nextComma + 1;
        }

        if (!seqFound) {
            result.parseError = ParseErrorType::MissingSeq;
            result.errorText = "SEQ_FIELD_NOT_FOUND";
            result.errorPos = 1 + commaPos + 1;
            return result;
        }

        result.ok = true;
        return result;
    }

    static RuntimeErrorInfo classifyMoveError(
        int N,
        int M,
        const int field[MAX_N][MAX_M],
        Pos from,
        Pos to,
        const MovementSettings& move,
        std::size_t stepIndex,
        char stepCmd
    ) {
        RuntimeErrorInfo info;
        info.stepIndex = stepIndex;
        info.stepCommand = stepCmd;
        info.position = from;

        int dx = to.x - from.x;
        int dy = to.y - from.y;

        if (!decodeStepCommand(stepCmd, dx, dy)) {
            info.type = RuntimeErrorType::InvalidStepCommand;
            info.text = "STEP_SYMBOL_IS_NOT_SUPPORTED";
            return info;
        }

        if (!isInside(N, M, to)) {
            info.type = RuntimeErrorType::OutOfBounds;
            info.text = "MOVE_OUTSIDE_MAZE";
            return info;
        }

        const bool diagonal = (std::abs(dx) == 1 && std::abs(dy) == 1);
        if (diagonal && !move.allowDiagonal) {
            info.type = RuntimeErrorType::DiagonalDisabled;
            info.text = "DIAGONAL_MOVES_DISABLED";
            return info;
        }

        if (!isPassableCell(field[to.y][to.x])) {
            info.type = RuntimeErrorType::Wall;
            info.text = "TARGET_CELL_IS_BLOCKED";
            return info;
        }

        if (diagonal && move.forbidCornerCutting) {
            const Pos side1{ from.x + dx, from.y };
            const Pos side2{ from.x, from.y + dy };
            if (!canStandOnCell(N, M, field, side1) || !canStandOnCell(N, M, field, side2)) {
                info.type = RuntimeErrorType::CornerCutting;
                info.text = "DIAGONAL_CORNER_CUTTING_FORBIDDEN";
                return info;
            }
        }

        info.type = RuntimeErrorType::None;
        return info;
    }

    static void resetSession(SessionState& s, Pos start) {
        clearBoolMask(s.visited);
        clearBoolMask(s.trail);
        s.robot = start;
        s.dir = DIR_RIGHT;
        s.visited[start.y][start.x] = true;
        s.executedPath.clear();
        s.stepCount = 0;
        s.errorCount = 0;
        s.pathCost = 0.0;
        s.ackRequired = false;
        s.showErrorBanner = false;
        s.screenErrorText.clear();
        s.lastError = RuntimeErrorInfo{};
    }

} // namespace

void runUartTrainingSession(
    int N,
    int M,
    const int field[MAX_N][MAX_M],
    Pos start,
    const Pos exits[MAX_EXITS],
    int exitCount,
    const MovementSettings& move,
    const char* scenarioPath
) {
    SessionState state;
    resetSession(state, start);
    SessionLogger logger = openSessionLogger(scenarioPath);

    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    if (logger.stream) {
        std::cout << "Журнал UART-сеанса: " << logger.filePath << "\n";
    }

    while (true) {
        const bool goal = isExit(exits, exitCount, state.robot);
        const std::string statusLine = makeStatusLine(state, field[state.robot.y][state.robot.x], goal);
        redraw(N, M, field, start, exits, exitCount, state, statusLine);

        if (goal) {
            emitPackets(logger, buildGoalPackets(state));
            logSummary(logger, state, true);
            std::cout << "Сеанс завершён: цель достигнута. Нажмите Enter для возврата в меню.\n";
            std::string line;
            std::getline(std::cin, line);
            return;
        }

        emitPacket(logger, buildReadyPacket(state, exits, exitCount, field));
        std::cout << "Введите UART-пакет: ";

        std::string line;
        std::getline(std::cin, line);
        logPacket(logger, "RX", line);

        ParsedPacket packet = parsePacket(line);
        if (!packet.ok) {
            state.errorCount += 1;
            state.ackRequired = true;
            state.lastError.type = RuntimeErrorType::AckRequired;
            state.lastError.text = packet.errorText;
            state.showErrorBanner = true;
            state.screenErrorText = makeParseErrorBannerText(packet);
            emitPacket(logger, buildParseErrorPacket(packet, state.errorCount));
            continue;
        }

        if (state.ackRequired &&
            packet.type != UartCommandType::Ack &&
            packet.type != UartCommandType::Help &&
            packet.type != UartCommandType::Exit) {
            state.errorCount += 1;
            RuntimeErrorInfo e;
            e.type = RuntimeErrorType::AckRequired;
            e.text = "ACK_IS_REQUIRED_BEFORE_NEW_COMMAND";
            e.position = state.robot;
            state.showErrorBanner = true;
            state.screenErrorText = makeRuntimeErrorBannerText(e);
            emitPacket(logger, buildRuntimeErrorPacket(e, state.errorCount));
            continue;
        }

        if (packet.type == UartCommandType::Ack) {
            state.ackRequired = false;
            state.showErrorBanner = false;
            state.screenErrorText.clear();
            state.lastError = RuntimeErrorInfo{};
            emitPacket(logger, buildAckOkPacket(state));
            continue;
        }

        if (packet.type == UartCommandType::Help) {
            emitPackets(logger, buildHelpPackets());
            emitPacket(logger, buildDetailPacket(state));
            std::cout << "Нажмите Enter для продолжения...\n";
            std::string dummy;
            std::getline(std::cin, dummy);
            continue;
        }

        if (packet.type == UartCommandType::Exit) {
            emitPacket(logger, "$STATE,STATUS=EXIT_TO_MENU*");
            logSummary(logger, state, false);
            return;
        }

        if (packet.type == UartCommandType::Status) {
            emitPacket(logger, buildStatePacket(state, exits, exitCount, field, "STATUS", '-'));
            emitPacket(logger, buildDetailPacket(state));
            continue;
        }

        if (packet.type == UartCommandType::Reset) {
            resetSession(state, start);
            redraw(
                N, M, field, start, exits, exitCount, state,
                "Сеанс сброшен командой CMD_RESET."
            );
            emitPacket(logger, buildStatePacket(state, exits, exitCount, field, "RESET_OK", '-'));
            emitPacket(logger, buildDetailPacket(state));
            continue;
        }

        // Команда CMD_MOVE: выполняем последовательность по одному шагу.
        bool executionStoppedByError = false;
        for (std::size_t i = 0; i < packet.sequence.size(); ++i) {
            const char stepCmd = packet.sequence[i];
            int dx = 0;
            int dy = 0;
            if (!decodeStepCommand(stepCmd, dx, dy)) {
                state.errorCount += 1;
                state.ackRequired = true;
                state.lastError.type = RuntimeErrorType::InvalidStepCommand;
                state.lastError.stepIndex = i;
                state.lastError.stepCommand = stepCmd;
                state.lastError.position = state.robot;
                state.lastError.text = "STEP_SYMBOL_IS_NOT_SUPPORTED";
                state.showErrorBanner = true;
                state.screenErrorText = makeRuntimeErrorBannerText(state.lastError);
                emitPacket(logger, buildRuntimeErrorPacket(state.lastError, state.errorCount));
                executionStoppedByError = true;
                break;
            }

            const Pos next{ state.robot.x + dx, state.robot.y + dy };
            RuntimeErrorInfo err = classifyMoveError(N, M, field, state.robot, next, move, i, stepCmd);
            if (err.type != RuntimeErrorType::None) {
                state.errorCount += 1;
                state.ackRequired = true;
                state.lastError = err;
                state.showErrorBanner = true;
                state.screenErrorText = makeRuntimeErrorBannerText(state.lastError);
                emitPacket(logger, buildRuntimeErrorPacket(state.lastError, state.errorCount));
                executionStoppedByError = true;
                break;
            }

            // Корректный шаг: обновляем состояние, путь и визуализацию.
            state.trail[state.robot.y][state.robot.x] = true;
            state.robot = next;
            state.dir = dirFromDelta(dx, dy);
            state.visited[state.robot.y][state.robot.x] = true;
            state.executedPath.push_back(stepCmd);
            state.stepCount += 1;
            state.pathCost += stepCost(field, Pos{ state.robot.x - dx, state.robot.y - dy }, state.robot);

            redraw(
                N, M, field, start, exits, exitCount, state,
                makeStatusLine(state, field[state.robot.y][state.robot.x], isExit(exits, exitCount, state.robot))
            );
            emitPacket(logger, buildStatePacket(state, exits, exitCount, field, "STEP_OK", stepCmd));

            if (isExit(exits, exitCount, state.robot)) {
                emitPackets(logger, buildGoalPackets(state));
                logSummary(logger, state, true);
                std::cout << "Сеанс завершён: цель достигнута. Нажмите Enter для возврата в меню.\n";
                std::string finish;
                std::getline(std::cin, finish);
                return;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(UART_STEP_DELAY_MS));
        }

        if (!executionStoppedByError) {
            emitPacket(logger, buildStatePacket(state, exits, exitCount, field, "CMD_OK", '-'));
            emitPacket(logger, buildDetailPacket(state));
        }
    }
}