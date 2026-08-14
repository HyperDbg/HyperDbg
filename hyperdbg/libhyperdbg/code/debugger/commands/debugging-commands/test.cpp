/**
 * @file test.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief test command
 * @details
 * @version 0.1
 * @date 2020-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"
#include <filesystem>
#include <mutex>
#include <regex>
#include <set>

namespace fs = std::filesystem;

//
// Global Variables
//
extern BOOLEAN g_IsKdModuleLoaded;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_CurrentExprEvalResultHasError;

struct CLI_SEMANTIC_EXPECTATION
{
    std::set<UINT32> Cases;
    std::set<std::string> Markers;
    std::string ExpectedError;
};

struct CLI_SEMANTIC_RESULT
{
    fs::path File;
    BOOLEAN Passed = FALSE;
    std::string Diagnostic;
    std::string Output;
    SIZE_T ExpectationCount = 0;
    UINT64 DurationMilliseconds = 0;
};

static std::mutex g_CliSemanticOutputMutex;
static std::string g_CliSemanticOutput;

static VOID
CommandTestCaptureSemanticOutput(CHAR* Message)
{
    if (!Message) return;
    std::lock_guard<std::mutex> Lock(g_CliSemanticOutputMutex);
    g_CliSemanticOutput.append(Message);
}

static std::string
CommandTestLowerAscii(std::string Value)
{
    std::transform(Value.begin(), Value.end(), Value.begin(), [](unsigned char Character) {
        return (CHAR)std::tolower(Character);
        });
    return Value;
}

static BOOLEAN
CommandTestParseSemanticFile(const fs::path& FilePath,
    const std::vector<std::string>& Arguments,
    std::string& Expression,
    CLI_SEMANTIC_EXPECTATION& Expectation,
    std::string& Error)
{
    std::ifstream File(FilePath, std::ios::binary);
    if (!File) { Error = "could not open file"; return FALSE; }
    std::string Content((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
    if (File.bad()) { Error = "could not read file"; return FALSE; }

    // Match .script argument semantics. Replace higher indexes first so an
    // argument such as $arg10 cannot be partially consumed as $arg1.
    for (SIZE_T Index = Arguments.size(); Index > 0; --Index)
        ReplaceAll(Content, "$arg" + std::to_string(Index - 1), Arguments[Index - 1]);

    Expectation = {};
    const std::regex CasePattern("test_case([0-9]+)[[:space:]]*=[[:space:]]*1");
    const std::regex MarkerPattern("//[[:space:]]*semantic-test-marker:[[:space:]]*([^\\r\\n]+)");
    const std::regex ErrorPattern("//[[:space:]]*semantic-test-expect-error:[[:space:]]*([^\\r\\n]+)");
    for (std::sregex_iterator Match(Content.begin(), Content.end(), CasePattern), End; Match != End; ++Match)
        Expectation.Cases.insert((UINT32)std::stoul((*Match)[1].str()));
    for (std::sregex_iterator Match(Content.begin(), Content.end(), MarkerPattern), End; Match != End; ++Match)
    {
        std::string Marker = (*Match)[1].str();
        while (!Marker.empty() && std::isspace((unsigned char)Marker.back())) Marker.pop_back();
        if (!Marker.empty()) Expectation.Markers.insert(Marker);
    }
    {
        std::smatch Match;
        if (std::regex_search(Content, Match, ErrorPattern))
        {
            Expectation.ExpectedError = Match[1].str();
            while (!Expectation.ExpectedError.empty() &&
                std::isspace((unsigned char)Expectation.ExpectedError.back()))
                Expectation.ExpectedError.pop_back();
        }
    }
    if (Expectation.Cases.empty() && Expectation.Markers.empty() && Expectation.ExpectedError.empty())
    {
        Error = "no enabled numbered case or semantic-test-marker";
        return FALSE;
    }

    SIZE_T CommandStart = std::string::npos;
    for (SIZE_T LineStart = 0; LineStart < Content.size();)
    {
        SIZE_T LineEnd = Content.find('\n', LineStart);
        SIZE_T First = Content.find_first_not_of(" \t\r", LineStart);
        if (First != std::string::npos && (LineEnd == std::string::npos || First < LineEnd) && Content[First] == '?')
        {
            CommandStart = First;
            break;
        }
        if (LineEnd == std::string::npos) break;
        LineStart = LineEnd + 1;
    }
    if (CommandStart == std::string::npos) { Error = "semantic script has no '?' command"; return FALSE; }
    SIZE_T ExpressionStart = Content.find_first_not_of(" \t\r\n", CommandStart + 1);
    if (ExpressionStart == std::string::npos) { Error = "semantic script has no expression"; return FALSE; }
    Expression.assign(Content, ExpressionStart, std::string::npos);
    return TRUE;
}

static BOOLEAN
CommandTestMalformedAggregatePush()
{
    UINT64 Stack[MAX_STACK_BUFFER_COUNT] = { 0 };
    UINT64 Globals[1] = { 0 };
    ACTION_BUFFER Action = { 0 };
    GUEST_REGS GuestRegs = { 0 };
    SYMBOL ErrorOperator = { 0 };

    auto RunMalformedCase = [&](SYMBOL* Symbols, UINT32 Count, UINT64 InitialStackIndex) {
        SYMBOL_BUFFER Buffer = { Symbols, Count, Count, NULL };
        SCRIPT_ENGINE_GENERAL_REGISTERS Registers = { Stack, Globals, InitialStackIndex, 0, 0 };
        UINT64 Index = 0;
        BOOLEAN HasError = ScriptEngineExecute(&GuestRegs, &Action, &Registers, &Buffer, &Index, &ErrorOperator);
        return HasError && Registers.StackIndx == InitialStackIndex;
        };

    SYMBOL MissingOperands[] = { {SYMBOL_SEMANTIC_RULE_TYPE, 0, FUNC_PUSH_AGGREGATE} };
    SYMBOL InvalidAddressSpace[] = {
        {SYMBOL_SEMANTIC_RULE_TYPE, 0, FUNC_PUSH_AGGREGATE},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, (UINT64)&Stack[0]},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, 99},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, 8} };
    SYMBOL InvalidSize[] = {
        {SYMBOL_SEMANTIC_RULE_TYPE, 0, FUNC_PUSH_AGGREGATE},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, (UINT64)&Stack[0]},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, 0} };
    SYMBOL StackOverflow[] = {
        {SYMBOL_SEMANTIC_RULE_TYPE, 0, FUNC_PUSH_AGGREGATE},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, (UINT64)&Stack[0]},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, 8} };
    SYMBOL InvalidSourceRange[] = {
        {SYMBOL_SEMANTIC_RULE_TYPE, 0, FUNC_PUSH_AGGREGATE},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, (UINT64)((BYTE*)&Stack[MAX_STACK_BUFFER_COUNT] - 4)},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL},
        {SYMBOL_NUM_TYPE, SYMBOL_VALUE_KIND_INTEGER, 8} };

    return RunMalformedCase(MissingOperands, ARRAYSIZE(MissingOperands), 7) &&
        RunMalformedCase(InvalidAddressSpace, ARRAYSIZE(InvalidAddressSpace), 7) &&
        RunMalformedCase(InvalidSize, ARRAYSIZE(InvalidSize), 7) &&
        RunMalformedCase(StackOverflow, ARRAYSIZE(StackOverflow), MAX_STACK_BUFFER_COUNT) &&
        RunMalformedCase(InvalidSourceRange, ARRAYSIZE(InvalidSourceRange), 7);
}

static BOOLEAN
CommandTestHasSemanticFailure(const std::string& Output)
{
    std::istringstream Lines(Output);
    std::string Line;
    while (std::getline(Lines, Line))
    {
        std::string Lower = CommandTestLowerAscii(Line);
        SIZE_T First = Lower.find_first_not_of(" \t\r");
        if (First == std::string::npos) continue;
        Lower.erase(0, First);
        if (Lower.find("was failed") != std::string::npos || Lower.rfind("err,", 0) == 0 ||
            Lower.rfind("error:", 0) == 0 || Lower.rfind("[x]", 0) == 0) return TRUE;
    }
    return FALSE;
}

static BOOLEAN
CommandTestHasExpectedSemanticOutput(const CLI_SEMANTIC_EXPECTATION& Expectation,
    const std::string& Output,
    std::string& Missing)
{
    if (CommandTestHasSemanticFailure(Output)) return FALSE;
    std::set<UINT32> SuccessfulCases;
    const std::regex Pattern("test case ([0-9]+) was successful");
    std::istringstream Lines(Output);
    std::string Line;
    while (std::getline(Lines, Line))
    {
        if (!Line.empty() && Line.back() == '\r') Line.pop_back();
        std::smatch Match;
        if (std::regex_match(Line, Match, Pattern)) SuccessfulCases.insert((UINT32)std::stoul(Match[1].str()));
    }
    for (UINT32 Case : Expectation.Cases)
    {
        if (!SuccessfulCases.contains(Case))
        {
            Missing = "missing test case " + std::to_string(Case) + " success output";
            return FALSE;
        }
    }
    for (const std::string& Marker : Expectation.Markers)
    {
        if (Output.find(Marker) == std::string::npos)
        {
            Missing = "missing semantic marker: " + Marker;
            return FALSE;
        }
    }
    return TRUE;
}

static VOID
CommandTestScriptSemantic()
{
    CHAR Directory[MAX_PATH] = { 0 };
    if (!SetupPathForFileName(SCRIPT_SEMANTIC_TEST_CASE_DIRECTORY, Directory, sizeof(Directory), FALSE))
    {
        ShowMessages("err, could not resolve the semantic test directory\n");
        return;
    }

    ShowMessages("[ RUN      ] malformed FUNC_PUSH_AGGREGATE validation\n");
    if (!CommandTestMalformedAggregatePush())
    {
        ShowMessages("[  FAILED  ] malformed FUNC_PUSH_AGGREGATE validation\n");
        return;
    }
    ShowMessages("[       OK ] malformed FUNC_PUSH_AGGREGATE validation (5 expectations)\n");

    std::vector<fs::path> Files;
    try
    {
        for (const fs::directory_entry& Entry : fs::directory_iterator(Directory))
            if (Entry.is_regular_file() && CommandTestLowerAscii(Entry.path().extension().string()) == ".ds")
                Files.push_back(Entry.path());
        std::sort(Files.begin(), Files.end(), [](const fs::path& Left, const fs::path& Right) {
            std::string L = CommandTestLowerAscii(Left.filename().string());
            std::string R = CommandTestLowerAscii(Right.filename().string());
            return L == R ? Left.filename().string() < Right.filename().string() : L < R;
            });
    }
    catch (const fs::filesystem_error& Exception)
    {
        ShowMessages("err, could not enumerate semantic tests: %s\n", Exception.what());
        return;
    }
    if (Files.empty()) { ShowMessages("err, no semantic .ds files were found in: %s\n", Directory); return; }

    std::vector<CLI_SEMANTIC_RESULT> Results;
    for (const fs::path& File : Files)
    {
        ShowMessages("[ RUN      ] %s\n", File.filename().string().c_str());
        CLI_SEMANTIC_RESULT Result;
        Result.File = File;
        CLI_SEMANTIC_EXPECTATION Expectation;
        std::string Expression;
        auto Started = std::chrono::steady_clock::now();
        const std::vector<std::string> Arguments = {
            File.string(),
            g_IsSerialConnectedToRemoteDebuggee ? "1" : "0",
            g_IsSerialConnectedToRemoteDebuggee ? "nt!ExAllocatePoolWithTag" : "0"
        };
        if (CommandTestParseSemanticFile(File, Arguments, Expression, Expectation, Result.Diagnostic))
        {
            Result.ExpectationCount = Expectation.Cases.size() + Expectation.Markers.size() +
                (Expectation.ExpectedError.empty() ? 0 : 1);
            {
                std::lock_guard<std::mutex> Lock(g_CliSemanticOutputMutex);
                g_CliSemanticOutput.clear();
            }
            BOOLEAN ExecutionSucceeded = TRUE;
            SetTextMessageCallback((PVOID)CommandTestCaptureSemanticOutput);
            if (g_IsSerialConnectedToRemoteDebuggee)
            {
                // Match the '?' command path in debugger mode so semantic cases
                // execute against the real debuggee registers and VMX-root
                // facilities instead of the simulated user-mode environment.
                ExecutionSucceeded = ScriptEngineExecuteSingleExpression(
                    (CHAR*)Expression.c_str(), TRUE, FALSE);
            }
            else
            {
                ScriptEngineWrapperTestParser(Expression);
                ExecutionSucceeded = !g_CurrentExprEvalResultHasError;
            }
            UnsetTextMessageCallback();
            {
                std::lock_guard<std::mutex> Lock(g_CliSemanticOutputMutex);
                Result.Output = g_CliSemanticOutput;
            }
            if (!Expectation.ExpectedError.empty())
            {
                std::string LowerOutput = CommandTestLowerAscii(Result.Output);
                std::string LowerExpected = CommandTestLowerAscii(Expectation.ExpectedError);
                Result.Passed = !ExecutionSucceeded &&
                    (LowerExpected == "any" || LowerOutput.find(LowerExpected) != std::string::npos);
                if (!Result.Passed)
                    Result.Diagnostic = ExecutionSucceeded ? "script unexpectedly succeeded" :
                    "expected error text was not reported";
            }
            else if (!ExecutionSucceeded)
                Result.Diagnostic = g_IsSerialConnectedToRemoteDebuggee ?
                "remote script evaluator reported an error" : "local script evaluator reported an error";
            else if (CommandTestHasSemanticFailure(Result.Output)) Result.Diagnostic = "semantic failure output was reported";
            else Result.Passed = CommandTestHasExpectedSemanticOutput(Expectation, Result.Output, Result.Diagnostic);
        }
        Result.DurationMilliseconds = (UINT64)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - Started).count();
        std::string PassDetails = Result.Passed ? std::to_string(Result.ExpectationCount) + " expectations, " : "";
        ShowMessages("[%s] %s (%s%llu ms)%s%s\n", Result.Passed ? "       OK " : "  FAILED  ",
            File.filename().string().c_str(), PassDetails.c_str(), Result.DurationMilliseconds,
            Result.Diagnostic.empty() ? "" : " - ", Result.Diagnostic.c_str());
        Results.push_back(std::move(Result));
    }

    SIZE_T Passed = std::count_if(Results.begin(), Results.end(), [](const CLI_SEMANTIC_RESULT& Result) { return Result.Passed; });
    ShowMessages("\nSemantic script suite: %llu files, %llu passed, %llu failed\n",
        (UINT64)Results.size(), (UINT64)Passed, (UINT64)(Results.size() - Passed));
    if (Passed != Results.size())
    {
        ShowMessages("Failed files:\n");
        for (const CLI_SEMANTIC_RESULT& Result : Results)
        {
            if (Result.Passed) continue;
            ShowMessages("  %s\n    %s\n", Result.File.filename().string().c_str(), Result.Diagnostic.c_str());
            if (!Result.Output.empty())
                ShowMessages("----- captured output -----\n%s%s----- end captured output -----\n",
                    Result.Output.c_str(), Result.Output.back() == '\n' ? "" : "\n");
        }
    }
}

/**
 * @brief help of the test command
 *
 * @return VOID
 */
VOID
CommandTestHelp()
{
    ShowMessages(
        "test : tests essential features of HyperDbg in current machine.\n");

    ShowMessages("syntax : \ttest [Task (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : test query\n");
    ShowMessages("\t\te.g : test trap-status\n");
    ShowMessages("\t\te.g : test pool\n");
    ShowMessages("\t\te.g : test query\n");
    ShowMessages("\t\te.g : test breakpoint on\n");
    ShowMessages("\t\te.g : test breakpoint off\n");
    ShowMessages("\t\te.g : test trap on\n");
    ShowMessages("\t\te.g : test trap off\n");
    ShowMessages("\t\te.g : test script-semantic\n");
}

/**
 * @brief Send an IOCTL to the kernel to run the
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandTestPerformKernelTestsIoctl()
{
    BOOL                          Status;
    ULONG                         ReturnedLength;
    DEBUGGER_PERFORM_KERNEL_TESTS KernelTestRequest = { 0 };

    AssertShowMessageReturnStmt(g_IsKdModuleLoaded, g_DeviceHandle, ASSERT_MESSAGE_KD_NOT_LOADED, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // By the way, we don't need to send an input buffer
    // to the kernel, but let's keep it like this, if we
    // want to pass some other arguments to the kernel in
    // the future
    //
    Status = PlatformDeviceIoControl(
        g_DeviceHandle,                       // Handle to device
        IOCTL_PERFORM_KERNEL_SIDE_TESTS,      // IO Control Code (IOCTL)
        &KernelTestRequest,                   // Input Buffer to driver.
        SIZEOF_DEBUGGER_PERFORM_KERNEL_TESTS, // Input buffer length
        &KernelTestRequest,                   // Output Buffer from driver.
        SIZEOF_DEBUGGER_PERFORM_KERNEL_TESTS, // Length of output buffer in
        // bytes.
        &ReturnedLength,                      // Bytes placed in buffer.
        NULL                                  // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", PlatformGetLastError());
        return FALSE;
    }

    if (KernelTestRequest.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        //
        // Nothing to show
        //
        return TRUE;
    }
    else
    {
        //
        // Show err message
        //
        ShowErrorMessage(KernelTestRequest.KernelStatus);
        return FALSE;
    }
}

/**
 * @brief perform test on for all functionalities
 *
 * @return VOID
 */
VOID
CommandTestAllFunctionalities()
{
    HANDLE ThreadHandle;
    HANDLE ProcessHandle;

    //
    // Test command parser
    //
    if (!OpenHyperDbgTestProcess(&ThreadHandle, &ProcessHandle, (CHAR*)TEST_CASE_PARAMETER_FOR_MAIN_COMMAND_PARSER))
    {
        ShowMessages("err, start HyperDbg test process for testing the main command parser\n");
        return;
    }

    //
    // Test PE parser helpers
    //
    if (!OpenHyperDbgTestProcess(&ThreadHandle, &ProcessHandle, (CHAR*)TEST_CASE_PARAMETER_FOR_PE_PARSER))
    {
        ShowMessages("err, start HyperDbg test process for testing the PE parser\n");
        return;
    }

    //
    // Test CodeView RSDS parser helpers
    //
    if (!OpenHyperDbgTestProcess(&ThreadHandle, &ProcessHandle, (CHAR*)TEST_CASE_PARAMETER_FOR_CODEVIEW_RSDS_PARSER))
    {
        ShowMessages("err, start HyperDbg test process for testing the CodeView RSDS parser\n");
        return;
    }

    //
    // Test script engine (script parser) using semantic tests
    //
    if (!OpenHyperDbgTestProcess(&ThreadHandle, &ProcessHandle, (CHAR*)TEST_CASE_PARAMETER_FOR_SCRIPT_SEMANTIC_TEST_CASES))
    {
        ShowMessages("err, start HyperDbg test process for testing semantic tests\n");
        return;
    }
}

/**
 * @brief perform test for all functionalities of hwdbg
 *
 * @return VOID
 */
VOID
CommandTestAllHwdbg()
{
    HANDLE ThreadHandle;
    HANDLE ProcessHandle;

    //
    // Test hwdbg functionalities
    //
    if (!OpenHyperDbgTestProcess(&ThreadHandle, &ProcessHandle, (CHAR*)TEST_HWDBG_FUNCTIONALITIES))
    {
        ShowMessages("err, start HyperDbg test process for testing hwdbg functionalities\n");
        return;
    }
}

/**
 * @brief perform test on the remote process
 *
 * @return BOOLEAN returns true if the results was true and false if the results
 * was not ok
 */
BOOLEAN
CommandTestPerformTest()
{
    BOOLEAN ResultOfTest = FALSE;
    HANDLE  PipeHandle;
    HANDLE  ThreadHandle;
    HANDLE  ProcessHandle;
    UINT32  ReadBytes;
    CHAR* Buffer = NULL;

    //
    // Allocate memory
    //
    Buffer = (CHAR*)malloc(TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    if (!Buffer)
    {
        ShowMessages("err, enable allocate communication buffer\n");
        return FALSE;
    }

    PlatformZeroMemory(Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    //
    // Create tests process to create a thread for us
    //
    if (!CreateProcessAndOpenPipeConnection(&PipeHandle,
        &ThreadHandle,
        &ProcessHandle))
    {
        ShowMessages("err, enable to connect to the test process\n");

        free(Buffer);

        return FALSE;
    }

    //
    // ***** Perform test specific routines *****
    //

    //
    // Wait for the result of test to be received
    //

SendCommandAndWaitForResponse:

    CHAR TestCommand[] = "this is a test command";

    BOOLEAN SentMessageResult = NamedPipeServerSendMessageToClient(
        PipeHandle,
        TestCommand,
        (UINT32)strlen(TestCommand) + 1);

    if (!SentMessageResult)
    {
        //
        // error in sending
        //
        return FALSE;
    }

    PlatformZeroMemory(Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);
    ReadBytes =
        NamedPipeServerReadClientMessage(PipeHandle, (CHAR*)Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    if (!ReadBytes)
    {
        //
        // Nothing to read
        //
        free(Buffer);

        return FALSE;
    }

    goto SendCommandAndWaitForResponse;

    //
    // Close connection and remote process
    //
    CloseProcessAndClosePipeConnection(PipeHandle, ThreadHandle, ProcessHandle);

    free(Buffer);

    return ResultOfTest;
}

/**
 * @brief test command for query the state
 *
 * @return VOID
 */
VOID
CommandTestQueryState()
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
            "in debugger mode\n");
        return;
    }

    //
    // Send the query to the debuggee
    //
    KdSendTestQueryPacketToDebuggee(TEST_QUERY_HALTING_CORE_STATUS);
}

/**
 * @brief test command for query the trap state
 *
 * @return VOID
 */
VOID
CommandTestQueryTrapState()
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
            "in debugger mode\n");
        return;
    }

    //
    // Send the query to the debuggee
    //
    KdSendTestQueryPacketToDebuggee(TEST_QUERY_TRAP_STATE);
}

/**
 * @brief test command for query the state of pre-allocated pools
 *
 * @return VOID
 */
VOID
CommandTestQueryPreAllocPoolsState()
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
            "in debugger mode\n");
        return;
    }

    //
    // Send the query to the debuggee
    //
    KdSendTestQueryPacketToDebuggee(TEST_QUERY_PREALLOCATED_POOL_STATE);
}

/**
 * @brief test command for setting target tasks to halted cores
 * @param Synchronous
 *
 * @return VOID
 */
VOID
CommandTestSetTargetTaskToHaltedCores(BOOLEAN Synchronous)
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
            "in debugger mode\n");
        return;
    }

    //
    // Send the target tasks to the halted cores
    //
    KdSendTestQueryPacketToDebuggee(Synchronous ? TEST_SETTING_TARGET_TASKS_ON_HALTED_CORES_SYNCHRONOUS : TEST_SETTING_TARGET_TASKS_ON_HALTED_CORES_ASYNCHRONOUS);
}

/**
 * @brief test command for setting target task to the specified core
 * @param CoreNumber
 *
 * @return VOID
 */
VOID
CommandTestSetTargetTaskToTargetCore(UINT32 CoreNumber)
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
            "in debugger mode\n");
        return;
    }

    //
    // Send the target task to the target halted core
    //
    KdSendTestQueryPacketWithContextToDebuggee(TEST_SETTING_TARGET_TASKS_ON_TARGET_HALTED_CORES, (UINT64)CoreNumber);
}

/**
 * @brief test command for turning on/off the breakpoints (#DB)
 * @param State
 * @return VOID
 */
VOID
CommandTestSetBreakpointState(BOOLEAN State)
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
            "in debugger mode\n");
        return;
    }

    //
    // Send the breakpoint settings to the debuggee
    //
    if (State)
    {
        KdSendTestQueryPacketToDebuggee(TEST_BREAKPOINT_TURN_ON_BPS);
    }
    else
    {
        KdSendTestQueryPacketToDebuggee(TEST_BREAKPOINT_TURN_OFF_BPS);
    }
}

/**
 * @brief test command for turning on/off the debug breaks (#DB)
 * @param State
 * @return VOID
 */
VOID
CommandTestSetDebugBreakState(BOOLEAN State)
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
            "in debugger mode\n");
        return;
    }

    //
    // Send the debug break settings to the debuggee
    //
    if (State)
    {
        KdSendTestQueryPacketToDebuggee(TEST_BREAKPOINT_TURN_ON_DBS);
    }
    else
    {
        KdSendTestQueryPacketToDebuggee(TEST_BREAKPOINT_TURN_OFF_DBS);
    }
}

/**
 * @brief test command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandTest(vector<CommandToken> CommandTokens, string Command)
{
    UINT64 Context = NULL;

    UINT32 CommandSize = (UINT32)CommandTokens.size();

    if (CommandSize == 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
            GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandTestHelp();
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "query"))
    {
        //
        // Query the state of debuggee in debugger mode
        //
        CommandTestQueryState();
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "trap-status"))
    {
        //
        // Query the state of trap flag in debugger mode
        //
        CommandTestQueryTrapState();
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "pool"))
    {
        //
        // Query the state of pre-allocated pools in debugger mode
        //
        CommandTestQueryPreAllocPoolsState();
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "sync-task"))
    {
        //
        // Send target task to the halted cores in debugger mode (synchronous)
        //
        CommandTestSetTargetTaskToHaltedCores(TRUE);
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "async-task"))
    {
        //
        // Send target task to the halted cores in debugger mode (asynchronous)
        //
        CommandTestSetTargetTaskToHaltedCores(FALSE);
    }
    else if (CommandSize == 3 && CompareLowerCaseStrings(CommandTokens.at(1), "target-core-task"))
    {
        if (!ConvertTokenToUInt64(CommandTokens.at(2), &Context))
        {
            ShowMessages("err, you should enter a valid hex number as the core id\n\n");
            return;
        }

        //
        // Send target task to the specific halted core in debugger mode
        //
        CommandTestSetTargetTaskToTargetCore((UINT32)Context);
    }
    else if (CommandSize == 3 && CompareLowerCaseStrings(CommandTokens.at(1), "breakpoint"))
    {
        //
        // Change breakpoint state
        //
        if (CompareLowerCaseStrings(CommandTokens.at(2), "on"))
        {
            CommandTestSetBreakpointState(TRUE);
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(2), "off"))
        {
            CommandTestSetBreakpointState(FALSE);
        }
        else
        {
            ShowMessages("err, couldn't resolve error at '%s'\n\n",
                GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2)).c_str());
            return;
        }
    }
    else if (CommandSize == 3 && CompareLowerCaseStrings(CommandTokens.at(1), "trap"))
    {
        //
        // Change debug break state
        //
        if (CompareLowerCaseStrings(CommandTokens.at(2), "on"))
        {
            CommandTestSetDebugBreakState(TRUE);
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(2), "off"))
        {
            CommandTestSetDebugBreakState(FALSE);
        }
        else
        {
            ShowMessages("err, couldn't resolve error at '%s'\n\n",
                GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2)).c_str());
            return;
        }
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "all"))
    {
        //
        // For testing functionalities
        //
        CommandTestAllFunctionalities();
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "script-semantic"))
    {
        CommandTestScriptSemantic();
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "hwdbg"))
    {
        //
        // For testing functionalities of hwdbg
        //
        CommandTestAllHwdbg();
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n",
            GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandTestHelp();
        return;
    }
}
