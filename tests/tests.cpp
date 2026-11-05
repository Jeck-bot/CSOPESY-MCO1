// Unit tests for the CSOPESY emulator.
// Usage, from the repo root: msbuild tests\CSOPESY.Tests.vcxproj /p:Configuration=Debug /p:Platform=x64,
// then run tests\x64\Debug\CSOPESY.Tests.exe. Expect "0 failures".

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// Included by path: putting src/ on the include path would make <thread>'s <process.h> resolve to
// Process.h, since Windows file names are case-insensitive.
#include "../src/Config.h"
#include "../src/Instruction.h"
#include "../src/Process.h"
#include "../src/Scheduler.h"
#include "../src/Utils.h"

namespace {

int checks = 0;
int failures = 0;
std::string currentTest;

void check(bool ok, const char* text, int line) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cout << "FAIL [" << currentTest << "] line " << line << ": " << text << "\n";
    }
}

struct Test {
    const char* name;
    void (*run)();
};

std::vector<Test>& registry() {
    static std::vector<Test> tests;
    return tests;
}

bool registerTest(const char* name, void (*run)()) {
    registry().push_back({name, run});
    return true;
}

// Local time as a time_t, so timestamp tests don't depend on the machine's time zone.
std::time_t localTime(int year, int month, int day, int hour, int minute, int second) {
    std::tm parts{};
    parts.tm_year = year - 1900;
    parts.tm_mon = month - 1;
    parts.tm_mday = day;
    parts.tm_hour = hour;
    parts.tm_min = minute;
    parts.tm_sec = second;
    parts.tm_isdst = -1;
    return std::mktime(&parts);
}

}  // namespace

#define CHECK(condition) check((condition), #condition, __LINE__)
#define TEST(name)                                                  \
    void name();                                                    \
    static const bool name##Registered = registerTest(#name, name); \
    void name()

// ---- Utils ----

TEST(formatsMorningTimeWithAm) {
    CHECK(formatTimestamp(localTime(2024, 1, 18, 9, 15, 22)) == "01/18/2024 09:15:22AM");
}

TEST(formatsEveningTimeWithPm) {
    CHECK(formatTimestamp(localTime(2024, 8, 6, 21, 5, 9)) == "08/06/2024 09:05:09PM");
}

TEST(formatsMidnightAsTwelveAm) {
    CHECK(formatTimestamp(localTime(2024, 12, 31, 0, 0, 0)) == "12/31/2024 12:00:00AM");
}

TEST(formatsNoonAsTwelvePm) {
    CHECK(formatTimestamp(localTime(2025, 3, 1, 12, 0, 7)) == "03/01/2025 12:00:07PM");
}

TEST(trimRemovesSurroundingWhitespace) {
    CHECK(trim("  screen -ls \t\r\n") == "screen -ls");
    CHECK(trim("   ") == "");
    CHECK(trim("") == "");
}

TEST(splitWordsIgnoresExtraSpaces) {
    std::vector<std::string> words = splitWords("  screen   -s  p01 ");
    CHECK(words.size() == 3);
    CHECK(words.size() == 3 && words[0] == "screen" && words[1] == "-s" && words[2] == "p01");
    CHECK(splitWords("   ").empty());
}

TEST(toLowerLowercasesLetters) {
    CHECK(toLower("Scheduler-START") == "scheduler-start");
}

// ---- Config ----

namespace {

bool parse(const std::string& text, Config& config, std::string& error) {
    std::istringstream input(text);
    return parseConfig(input, config, error);
}

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

}  // namespace

TEST(parsesSpecSampleConfig) {
    Config config;
    std::string error;
    CHECK(parse("num-cpu 4\nscheduler \"rr\"\nquantum-cycles 5\nbatch-process-freq 1\n"
                "min-ins 1000\nmax-ins 2000\ndelay-per-exec 0\n",
                config, error));
    CHECK(config.numCpu == 4);
    CHECK(config.scheduler == SchedulerType::RR);
    CHECK(config.quantumCycles == 5);
    CHECK(config.batchProcessFreq == 1);
    CHECK(config.minIns == 1000);
    CHECK(config.maxIns == 2000);
    CHECK(config.delayPerExec == 0);
}

TEST(parsesConfigWrittenOnOneLine) {
    Config config;
    std::string error;
    CHECK(parse("num-cpu 8 scheduler fcfs quantum-cycles 3 batch-process-freq 7 min-ins 10 max-ins 20 "
                "delay-per-exec 2",
                config, error));
    CHECK(config.numCpu == 8);
    CHECK(config.scheduler == SchedulerType::FCFS);
    CHECK(config.quantumCycles == 3);
    CHECK(config.batchProcessFreq == 7);
    CHECK(config.minIns == 10);
    CHECK(config.maxIns == 20);
    CHECK(config.delayPerExec == 2);
}

TEST(acceptsSchedulerInAnyCaseWithOrWithoutQuotes) {
    Config config;
    std::string error;
    CHECK(parse("scheduler \"FCFS\"", config, error));
    CHECK(config.scheduler == SchedulerType::FCFS);
    CHECK(parse("scheduler RR", config, error));
    CHECK(config.scheduler == SchedulerType::RR);
}

TEST(acceptsDelaysPerExecSpelling) {
    Config config;
    std::string error;
    CHECK(parse("delays-per-exec 7", config, error));
    CHECK(config.delayPerExec == 7);
}

TEST(usesSpecDefaultsForMissingKeys) {
    Config config;
    config.numCpu = 99;
    std::string error;
    CHECK(parse("min-ins 1", config, error));
    CHECK(config.numCpu == 4);
    CHECK(config.scheduler == SchedulerType::RR);
    CHECK(config.quantumCycles == 5);
    CHECK(config.batchProcessFreq == 1);
    CHECK(config.minIns == 1);
    CHECK(config.maxIns == 2000);
    CHECK(config.delayPerExec == 0);
}

TEST(acceptsRangeLimits) {
    Config config;
    std::string error;
    CHECK(parse("num-cpu 1", config, error));
    CHECK(parse("num-cpu 128", config, error));
    CHECK(parse("delay-per-exec 0", config, error));
    CHECK(parse("quantum-cycles 4294967296 batch-process-freq 4294967296 min-ins 4294967296 "
                "max-ins 4294967296 delay-per-exec 4294967296",
                config, error));
    CHECK(config.quantumCycles == 4294967296ULL);
    CHECK(config.batchProcessFreq == 4294967296ULL);
    CHECK(config.minIns == 4294967296ULL);
    CHECK(config.maxIns == 4294967296ULL);
    CHECK(config.delayPerExec == 4294967296ULL);
}

TEST(rejectsValuesOutsideTheRanges) {
    Config config;
    std::string error;
    CHECK(!parse("num-cpu 0", config, error));
    CHECK(!parse("num-cpu 129", config, error));
    CHECK(!parse("quantum-cycles 0", config, error));
    CHECK(!parse("quantum-cycles 4294967297", config, error));
    CHECK(!parse("batch-process-freq 0", config, error));
    CHECK(!parse("min-ins 0", config, error));
    CHECK(!parse("max-ins 4294967297", config, error));
    CHECK(!parse("delay-per-exec 4294967297", config, error));
}

TEST(rejectsNonNumericValues) {
    Config config;
    std::string error;
    CHECK(!parse("num-cpu four", config, error));
    CHECK(!parse("num-cpu -1", config, error));
    CHECK(!parse("min-ins 12abc", config, error));
    CHECK(!parse("max-ins 99999999999999999999999", config, error));
}

TEST(rejectsUnknownScheduler) {
    Config config;
    std::string error;
    CHECK(!parse("scheduler sjf", config, error));
    CHECK(contains(error, "scheduler"));
}

TEST(rejectsMinInsAboveMaxIns) {
    Config config;
    std::string error;
    CHECK(!parse("min-ins 10 max-ins 5", config, error));
    CHECK(contains(error, "min-ins"));
}

TEST(rejectsKeyWithoutValue) {
    Config config;
    std::string error;
    CHECK(!parse("num-cpu 4 scheduler", config, error));
    CHECK(contains(error, "scheduler"));
}

TEST(errorNamesTheBadKey) {
    Config config;
    std::string error;
    CHECK(!parse("num-cpu 0", config, error));
    CHECK(contains(error, "num-cpu"));
}

TEST(leavesConfigUnchangedOnError) {
    Config config;
    config.numCpu = 7;
    std::string error;
    CHECK(!parse("num-cpu 2 max-ins 0", config, error));
    CHECK(config.numCpu == 7);
}

TEST(ignoresUnknownKeys) {
    Config config;
    std::string error;
    CHECK(parse("colour blue num-cpu 2", config, error));
    CHECK(config.numCpu == 2);
}

TEST(loadConfigReadsAFile) {
    {
        std::ofstream file("test-config.txt");
        file << "num-cpu 3\nscheduler \"fcfs\"\n";
    }
    Config config;
    std::string error;
    CHECK(loadConfig("test-config.txt", config, error));
    CHECK(config.numCpu == 3);
    CHECK(config.scheduler == SchedulerType::FCFS);
    std::remove("test-config.txt");
}

TEST(loadConfigReportsMissingFile) {
    Config config;
    std::string error;
    CHECK(!loadConfig("no-such-config-file.txt", config, error));
    CHECK(contains(error, "no-such-config-file.txt"));
}

// ---- Instruction ----

namespace {

int loopDepth(const Instruction& statement) {
    if (statement.type != InstructionType::FOR) {
        return 0;
    }
    int deepest = 0;
    for (const Instruction& child : statement.body) {
        deepest = std::max(deepest, loopDepth(child));
    }
    return 1 + deepest;
}

bool loopsAreWellFormed(const Instruction& statement) {
    if (statement.type != InstructionType::FOR) {
        return true;
    }
    if (statement.repeats < 2 || statement.body.empty()) {
        return false;
    }
    for (const Instruction& child : statement.body) {
        if (!loopsAreWellFormed(child)) {
            return false;
        }
    }
    return true;
}

bool isPoolVariable(const std::string& name) {
    return name == "x" || name == "y" || name == "z";
}

Instruction makeInstruction(InstructionType type) {
    Instruction instruction;
    instruction.type = type;
    return instruction;
}

Instruction makeLoop(uint16_t repeats, std::vector<Instruction> body) {
    Instruction loop = makeInstruction(InstructionType::FOR);
    loop.repeats = repeats;
    loop.body = std::move(body);
    return loop;
}

}  // namespace

TEST(executedCountMultipliesNestedLoops) {
    Instruction print = makeInstruction(InstructionType::PRINT);
    Instruction inner = makeLoop(2, {print, print});
    Instruction outer = makeLoop(3, {print, inner});
    CHECK(executedCount(print) == 1);
    CHECK(executedCount(inner) == 4);
    CHECK(executedCount(outer) == 15);
}

TEST(singleInstructionBudgetNeverMakesALoop) {
    bool noLoop = true;
    for (uint32_t seed = 1; seed <= 500; ++seed) {
        std::minstd_rand rng(seed);
        Instruction statement = randomStatement(rng, "p01", 0, 1);
        noLoop = noLoop && statement.type != InstructionType::FOR && executedCount(statement) == 1;
    }
    CHECK(noLoop);
}

TEST(randomStatementsStayWithinTheirBudget) {
    const uint64_t budgets[] = {2, 3, 7, 50, 1000, 4294967296ULL};
    bool withinBudget = true;
    bool wellFormed = true;
    bool shallowEnough = true;
    for (uint64_t budget : budgets) {
        for (uint32_t seed = 1; seed <= 2000; ++seed) {
            std::minstd_rand rng(seed);
            Instruction statement = randomStatement(rng, "p01", 0, budget);
            uint64_t count = executedCount(statement);
            withinBudget = withinBudget && count >= 1 && count <= budget;
            wellFormed = wellFormed && loopsAreWellFormed(statement);
            shallowEnough = shallowEnough && loopDepth(statement) <= kMaxForNesting;
        }
    }
    CHECK(withinBudget);
    CHECK(wellFormed);
    CHECK(shallowEnough);
}

TEST(generatesEveryInstructionType) {
    bool seen[6] = {};
    std::minstd_rand rng(42);
    for (int i = 0; i < 600; ++i) {
        seen[static_cast<int>(randomStatement(rng, "p01", 0, 1000).type)] = true;
    }
    CHECK(seen[0] && seen[1] && seen[2] && seen[3] && seen[4] && seen[5]);
}

TEST(neverStartsALoopAtTheNestingLimit) {
    bool noLoop = true;
    std::minstd_rand rng(7);
    for (int i = 0; i < 1000; ++i) {
        noLoop = noLoop && randomStatement(rng, "p01", kMaxForNesting, 1000).type != InstructionType::FOR;
    }
    CHECK(noLoop);
}

TEST(printUsesTheDefaultHelloWorldMessage) {
    bool foundPrint = false;
    bool defaultMessage = true;
    std::minstd_rand rng(3);
    for (int i = 0; i < 300; ++i) {
        Instruction statement = randomStatement(rng, "screen_01", 0, 1);
        if (statement.type == InstructionType::PRINT) {
            foundPrint = true;
            defaultMessage = defaultMessage && statement.message == "Hello world from screen_01!" &&
                             statement.variable.empty();
        }
    }
    CHECK(foundPrint);
    CHECK(defaultMessage);
}

TEST(sleepLastsOneToTenTicks) {
    bool foundSleep = false;
    bool inRange = true;
    std::minstd_rand rng(5);
    for (int i = 0; i < 300; ++i) {
        Instruction statement = randomStatement(rng, "p01", 0, 1);
        if (statement.type == InstructionType::SLEEP) {
            foundSleep = true;
            inRange = inRange && statement.sleepTicks >= 1 && statement.sleepTicks <= 10;
        }
    }
    CHECK(foundSleep);
    CHECK(inRange);
}

TEST(arithmeticUsesVariablesFromThePool) {
    bool foundArithmetic = false;
    bool valid = true;
    std::minstd_rand rng(11);
    for (int i = 0; i < 300; ++i) {
        Instruction statement = randomStatement(rng, "p01", 0, 1);
        if (statement.type == InstructionType::DECLARE) {
            valid = valid && isPoolVariable(statement.variable);
        }
        if (statement.type == InstructionType::ADD || statement.type == InstructionType::SUBTRACT) {
            foundArithmetic = true;
            valid = valid && isPoolVariable(statement.variable) &&
                    (!statement.first.isVariable || isPoolVariable(statement.first.variable)) &&
                    (!statement.second.isVariable || isPoolVariable(statement.second.variable));
        }
    }
    CHECK(foundArithmetic);
    CHECK(valid);
}

// ---- Process ----

namespace {

Instruction makePrint(const std::string& message, const std::string& variable = "") {
    Instruction print = makeInstruction(InstructionType::PRINT);
    print.message = message;
    print.variable = variable;
    return print;
}

Instruction makeDeclare(const std::string& variable, uint16_t value) {
    Instruction declare = makeInstruction(InstructionType::DECLARE);
    declare.variable = variable;
    declare.first.value = value;
    return declare;
}

Operand literal(uint16_t value) {
    Operand operand;
    operand.value = value;
    return operand;
}

Operand variableOperand(const std::string& name) {
    Operand operand;
    operand.isVariable = true;
    operand.variable = name;
    return operand;
}

Instruction makeArithmetic(InstructionType type, const std::string& target, Operand first, Operand second) {
    Instruction arithmetic = makeInstruction(type);
    arithmetic.variable = target;
    arithmetic.first = first;
    arithmetic.second = second;
    return arithmetic;
}

Instruction makeSleep(uint8_t ticks) {
    Instruction sleep = makeInstruction(InstructionType::SLEEP);
    sleep.sleepTicks = ticks;
    return sleep;
}

bool hasValue(const Process& process, const std::string& variable, uint16_t expected) {
    auto found = process.getVariables().find(variable);
    return found != process.getVariables().end() && found->second == expected;
}

}  // namespace

TEST(newProcessStartsAtLineZero) {
    std::time_t before = std::time(nullptr);
    Process process(7, "screen_01", 50, 1);
    std::time_t after = std::time(nullptr);
    CHECK(process.getId() == 7);
    CHECK(process.getName() == "screen_01");
    CHECK(process.getTotalInstructions() == 50);
    CHECK(process.getExecuted() == 0);
    CHECK(!process.isFinished());
    CHECK(process.getCreatedAt() >= before && process.getCreatedAt() <= after);
}

TEST(declareSetsTheVariable) {
    Process process(1, "p", 10, 1);
    process.execute(makeDeclare("x", 5), 0);
    CHECK(hasValue(process, "x", 5));
}

TEST(addStoresTheSum) {
    Process process(1, "p", 10, 1);
    process.execute(makeArithmetic(InstructionType::ADD, "x", literal(3), literal(4)), 0);
    CHECK(hasValue(process, "x", 7));
}

TEST(addClampsAtUint16Max) {
    Process process(1, "p", 10, 1);
    process.execute(makeArithmetic(InstructionType::ADD, "x", literal(65000), literal(1000)), 0);
    CHECK(hasValue(process, "x", 65535));
}

TEST(subtractClampsAtZero) {
    Process process(1, "p", 10, 1);
    process.execute(makeArithmetic(InstructionType::SUBTRACT, "x", literal(5), literal(10)), 0);
    CHECK(hasValue(process, "x", 0));
}

TEST(arithmeticReadsVariables) {
    Process process(1, "p", 10, 1);
    process.execute(makeDeclare("y", 10), 0);
    process.execute(makeArithmetic(InstructionType::ADD, "x", variableOperand("y"), variableOperand("y")), 0);
    process.execute(makeArithmetic(InstructionType::SUBTRACT, "z", variableOperand("x"), literal(5)), 0);
    CHECK(hasValue(process, "x", 20));
    CHECK(hasValue(process, "z", 15));
}

TEST(undeclaredVariablesAreDeclaredAsZero) {
    Process process(1, "p", 10, 1);
    process.execute(makeArithmetic(InstructionType::ADD, "x", variableOperand("y"), literal(1)), 0);
    CHECK(hasValue(process, "x", 1));
    CHECK(hasValue(process, "y", 0));
}

TEST(printLogsTheMessageWithCoreAndTime) {
    Process process(1, "p01", 10, 1);
    std::time_t before = std::time(nullptr);
    process.execute(makePrint("Hello world from p01!"), 2);
    std::time_t after = std::time(nullptr);
    CHECK(process.getLogs().size() == 1);
    if (process.getLogs().size() == 1) {
        const LogEntry& log = process.getLogs()[0];
        CHECK(log.text == "Hello world from p01!");
        CHECK(log.core == 2);
        CHECK(log.time >= before && log.time <= after);
    }
}

TEST(printAppendsTheVariableValue) {
    Process process(1, "p", 10, 1);
    process.execute(makeDeclare("x", 5), 0);
    process.execute(makePrint("Value from: ", "x"), 0);
    process.execute(makePrint("Undeclared: ", "w"), 0);
    CHECK(process.getLogs().size() == 2);
    if (process.getLogs().size() == 2) {
        CHECK(process.getLogs()[0].text == "Value from: 5");
        CHECK(process.getLogs()[1].text == "Undeclared: 0");
    }
}

TEST(loopRepeatsItsBody) {
    Process process(1, "p", 3, 1);
    process.startLoop(makeLoop(3, {makePrint("hi")}));
    CHECK(process.step(0) == StepResult::RAN);
    CHECK(process.step(0) == StepResult::RAN);
    CHECK(process.step(0) == StepResult::FINISHED);
    CHECK(process.getLogs().size() == 3);
    CHECK(process.getExecuted() == 3);
    CHECK(process.isFinished());
}

TEST(nestedLoopsRunEveryRepetition) {
    Process process(1, "p", 9, 1);
    process.startLoop(makeLoop(3, {makePrint("outer"), makeLoop(2, {makeDeclare("x", 1)})}));
    int steps = 0;
    StepResult result = StepResult::RAN;
    while (result != StepResult::FINISHED && steps < 20) {
        result = process.step(0);
        ++steps;
    }
    CHECK(steps == 9);
    CHECK(process.getLogs().size() == 3);
}

TEST(sleepReturnsSleptWithItsTicks) {
    Process process(1, "p", 2, 1);
    process.startLoop(makeLoop(2, {makeSleep(4)}));
    CHECK(process.step(0) == StepResult::SLEPT);
    CHECK(process.getSleepTicks() == 4);
    CHECK(process.step(0) == StepResult::FINISHED);
}

TEST(randomProcessesRunExactlyTheirLength) {
    const uint64_t lengths[] = {1, 2, 3, 17, 1000};
    bool exact = true;
    bool released = true;
    for (uint64_t length : lengths) {
        for (uint32_t seed = 1; seed <= 200; ++seed) {
            Process process(1, "p", length, seed);
            uint64_t steps = 0;
            while (!process.isFinished() && steps <= length) {
                process.step(static_cast<int>(steps % 4));
                ++steps;
            }
            exact = exact && steps == length && process.getExecuted() == length && process.isFinished();
            released = released && process.getVariables().empty();
        }
    }
    CHECK(exact);
    CHECK(released);
}

TEST(processesAlternatePrintAndAddOnX) {
    Process process(1, "p", 100, 3);
    CHECK(hasValue(process, "x", 0));
    uint16_t x = 0;
    bool alternates = true;
    for (int i = 0; i < 100; ++i) {
        size_t logsBefore = process.getLogs().size();
        process.step(0);
        if (i % 2 == 0) {
            alternates = alternates && process.getLogs().size() == logsBefore + 1 &&
                         process.getLogs().back().text == "Value from: " + std::to_string(x);
        } else if (i < 99) {
            uint16_t now = process.getVariables().at("x");
            alternates = alternates && process.getLogs().size() == logsBefore && now >= x + 1 && now <= x + 10;
            x = now;
        }
    }
    CHECK(alternates);
    CHECK(process.isFinished());
}

// ---- Scheduler ----

namespace {

Config testConfig(uint32_t cores, SchedulerType type, uint64_t quantum, uint64_t minIns, uint64_t maxIns,
                  uint64_t delay, uint64_t batchFrequency = 1) {
    Config config;
    config.numCpu = cores;
    config.scheduler = type;
    config.quantumCycles = quantum;
    config.minIns = minIns;
    config.maxIns = maxIns;
    config.delayPerExec = delay;
    config.batchProcessFreq = batchFrequency;
    return config;
}

std::string onFirstCore(const Utilization& usage) {
    return usage.running.empty() ? "" : usage.running[0].name;
}

uint64_t executedOnFirstCore(const Utilization& usage) {
    return usage.running.empty() ? 0 : usage.running[0].executed;
}

uint64_t executedOf(const Scheduler& scheduler, const std::string& name) {
    std::optional<ProcessView> view = scheduler.view(name);
    return view ? view->executed : 0;
}

bool hasFinished(const Scheduler& scheduler, const std::string& name) {
    std::optional<ProcessView> view = scheduler.view(name);
    return view && view->finished;
}

}  // namespace

TEST(createsProcessesWithUniqueNames) {
    Scheduler scheduler(testConfig(1, SchedulerType::FCFS, 5, 10, 10, 0), 1);
    CHECK(scheduler.createProcess("a"));
    CHECK(!scheduler.createProcess("a"));
    std::optional<ProcessView> view = scheduler.view("a");
    CHECK(view.has_value());
    if (view) {
        CHECK(view->name == "a");
        CHECK(view->id == 1);
        CHECK(view->total == 10);
        CHECK(view->executed == 0);
        CHECK(!view->finished);
    }
    CHECK(!scheduler.view("missing").has_value());
}

TEST(processLengthsFollowMinAndMaxIns) {
    Scheduler scheduler(testConfig(1, SchedulerType::FCFS, 5, 3, 7, 0), 1);
    bool inRange = true;
    for (int i = 0; i < 100; ++i) {
        std::string name = "proc" + std::to_string(i);
        scheduler.createProcess(name);
        std::optional<ProcessView> view = scheduler.view(name);
        inRange = inRange && view && view->total >= 3 && view->total <= 7;
    }
    CHECK(inRange);
}

TEST(idleCoresTakeReadyProcessesInOrder) {
    Scheduler scheduler(testConfig(2, SchedulerType::FCFS, 5, 1000, 1000, 0), 1);
    scheduler.createProcess("a");
    scheduler.createProcess("b");
    scheduler.createProcess("c");
    scheduler.tick();
    Utilization usage = scheduler.utilization();
    CHECK(usage.coreCount == 2);
    CHECK(usage.coresUsed == 2);
    CHECK(usage.running.size() == 2);
    if (usage.running.size() == 2) {
        CHECK(usage.running[0].name == "a" && usage.running[0].core == 0);
        CHECK(usage.running[1].name == "b" && usage.running[1].core == 1);
    }
    CHECK(usage.finished.empty());
}

TEST(batchGenerationCreatesAProcessEveryFrequencyTicks) {
    Scheduler scheduler(testConfig(1, SchedulerType::FCFS, 5, 1000, 1000, 0, 3), 1);
    CHECK(scheduler.startBatchGeneration());
    CHECK(!scheduler.startBatchGeneration());
    for (int i = 0; i < 9; ++i) {
        scheduler.tick();
    }
    CHECK(scheduler.view("p01").has_value());
    CHECK(scheduler.view("p02").has_value());
    CHECK(scheduler.view("p03").has_value());
    CHECK(!scheduler.view("p04").has_value());
    CHECK(scheduler.stopBatchGeneration());
    CHECK(!scheduler.stopBatchGeneration());
    for (int i = 0; i < 30; ++i) {
        scheduler.tick();
    }
    CHECK(!scheduler.view("p04").has_value());
}

TEST(batchNamesSkipTakenNamesAndGrowPastTwoDigits) {
    Scheduler scheduler(testConfig(1, SchedulerType::FCFS, 5, 1000, 1000, 0, 1), 1);
    scheduler.createProcess("p01");
    scheduler.startBatchGeneration();
    for (int i = 0; i < 100; ++i) {
        scheduler.tick();
    }
    CHECK(scheduler.view("p02").has_value());
    CHECK(scheduler.view("p100").has_value());
    CHECK(scheduler.view("p101").has_value());
    CHECK(!scheduler.view("p102").has_value());
}

TEST(everyProcessFinishesAfterExactlyItsLength) {
    Scheduler scheduler(testConfig(4, SchedulerType::RR, 2, 1, 60, 0, 1), 3);
    scheduler.startBatchGeneration();
    for (int i = 0; i < 200; ++i) {
        scheduler.tick();
    }
    scheduler.stopBatchGeneration();
    for (int i = 0; i < 200000 && scheduler.utilization().finished.size() < 200; ++i) {
        scheduler.tick();
    }
    Utilization usage = scheduler.utilization();
    CHECK(usage.finished.size() == 200);
    bool exact = true;
    for (const ProcessRow& row : usage.finished) {
        exact = exact && row.executed == row.total && row.core == -1;
    }
    CHECK(exact);
    CHECK(usage.coresUsed == 0);
    CHECK(usage.running.empty());
}

TEST(noProcessRunsOnTwoCoresAtOnce) {
    Scheduler scheduler(testConfig(128, SchedulerType::RR, 2, 1, 50, 0, 1), 5);
    scheduler.startBatchGeneration();
    bool consistent = true;
    for (int i = 0; i < 1500; ++i) {
        if (i == 1000) {
            scheduler.stopBatchGeneration();
        }
        scheduler.tick();
        Utilization usage = scheduler.utilization();
        std::set<std::string> names;
        int previousCore = -1;
        for (const ProcessRow& row : usage.running) {
            names.insert(row.name);
            consistent = consistent && row.core > previousCore && row.core < 128;
            previousCore = row.core;
        }
        consistent = consistent && names.size() == usage.running.size() &&
                     usage.coresUsed == usage.running.size() && usage.coreCount == 128;
    }
    CHECK(consistent);
}

TEST(fcfsNeverPreemptsARunningProcess) {
    // With a long delay, a preempted process would leave while busy-waiting, without having run
    // an instruction. Under FCFS a process only leaves right after running SLEEP or its last line.
    Scheduler scheduler(testConfig(1, SchedulerType::FCFS, 2, 20, 20, 50), 9);
    scheduler.createProcess("a");
    scheduler.createProcess("b");
    scheduler.createProcess("c");
    std::string previous;
    uint64_t previousExecuted = 0;
    bool neverPreempted = true;
    for (int i = 0; i < 20000 && scheduler.utilization().finished.size() < 3; ++i) {
        scheduler.tick();
        Utilization usage = scheduler.utilization();
        std::string current = onFirstCore(usage);
        if (!previous.empty() && current != previous) {
            std::optional<ProcessView> view = scheduler.view(previous);
            neverPreempted = neverPreempted && view && view->executed > previousExecuted;
        }
        previous = current;
        previousExecuted = executedOnFirstCore(usage);
    }
    CHECK(neverPreempted);
    CHECK(scheduler.utilization().finished.size() == 3);
}

TEST(roundRobinPreemptsAfterTheQuantum) {
    // A process may keep the core past its quantum only while the other one sleeps (at most
    // 10 ticks), so with two busy processes no run lasts much longer than the quantum.
    Scheduler scheduler(testConfig(1, SchedulerType::RR, 3, 20, 20, 50), 9);
    scheduler.createProcess("a");
    scheduler.createProcess("b");
    std::string previous;
    uint64_t previousExecuted = 0;
    int run = 0;
    int longestRun = 0;
    int preemptions = 0;
    for (int i = 0; i < 20000 && scheduler.utilization().finished.size() < 2; ++i) {
        scheduler.tick();
        Utilization usage = scheduler.utilization();
        std::string current = onFirstCore(usage);
        run = (!current.empty() && current == previous) ? run + 1 : (current.empty() ? 0 : 1);
        if (usage.finished.empty()) {
            longestRun = std::max(longestRun, run);
        }
        if (!previous.empty() && current != previous) {
            std::optional<ProcessView> view = scheduler.view(previous);
            if (view && !view->finished && view->executed == previousExecuted) {
                ++preemptions;
            }
        }
        previous = current;
        previousExecuted = executedOnFirstCore(usage);
    }
    CHECK(longestRun <= 3 + 12);
    CHECK(preemptions > 0);
    CHECK(scheduler.utilization().finished.size() == 2);
}

TEST(delayPerExecSpacesInstructions) {
    Scheduler scheduler(testConfig(1, SchedulerType::FCFS, 5, 30, 30, 3), 4);
    scheduler.createProcess("a");
    uint64_t lastExecuted = 0;
    int lastChange = -1000;
    bool spaced = true;
    for (int t = 0; t < 2000; ++t) {
        scheduler.tick();
        uint64_t executed = executedOf(scheduler, "a");
        if (executed != lastExecuted) {
            spaced = spaced && t - lastChange >= 4;
            lastChange = t;
            lastExecuted = executed;
        }
    }
    CHECK(spaced);
    CHECK(hasFinished(scheduler, "a"));
}

TEST(backgroundThreadRunsTicksUntilStopped) {
    Scheduler scheduler(testConfig(1, SchedulerType::FCFS, 5, 1000000, 1000000, 0), 1);
    scheduler.createProcess("a");
    scheduler.start();
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (executedOf(scheduler, "a") < 5 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    scheduler.stop();
    uint64_t executed = executedOf(scheduler, "a");
    CHECK(executed >= 5);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(executedOf(scheduler, "a") == executed);
}

int main() {
    for (const Test& test : registry()) {
        currentTest = test.name;
        test.run();
    }
    std::cout << registry().size() << " tests, " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
