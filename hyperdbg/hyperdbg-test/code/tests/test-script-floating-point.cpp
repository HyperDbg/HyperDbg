/**
 * @file test-script-floating-point.cpp
 * @brief Focused scanner, IR, evaluator, and formatting tests for floating-point scripts.
 */
#include "pch.h"

static std::string CapturedScriptOutput;

static VOID
CaptureScriptOutput(CHAR * Message)
{
    if (Message)
    {
        CapturedScriptOutput.append(Message);
    }
}

static BOOLEAN
RunScriptAndExpect(const CHAR * Script, BOOLEAN ExpectedSuccess, const CHAR * ExpectedOutput)
{
    CapturedScriptOutput.clear();
    hyperdbg_u_set_text_message_callback((PVOID)CaptureScriptOutput);
    BOOLEAN Result = hyperdbg_u_test_script_engine((CHAR *)Script);
    hyperdbg_u_unset_text_message_callback();

    if (Result != ExpectedSuccess)
    {
        std::cerr << "Unexpected script result for: " << Script
                  << "\nExpected success: " << (ExpectedSuccess ? "true" : "false")
                  << "\nActual output: " << CapturedScriptOutput << std::endl;
        return FALSE;
    }

    if (ExpectedOutput && CapturedScriptOutput != ExpectedOutput)
    {
        std::cerr << "Unexpected script output for: " << Script
                  << "\nExpected: " << ExpectedOutput
                  << "\nActual: " << CapturedScriptOutput << std::endl;
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN
RunScriptAndExpectRuntimeFailure(const CHAR * Script)
{
    if (!RunScriptAndExpect(Script, FALSE, NULL)) return FALSE;
    if (CapturedScriptOutput.find("ScriptEngineExecute") == std::string::npos)
    {
        std::cerr << "Expected an evaluator failure for: " << Script
                  << "\nActual output: " << CapturedScriptOutput << std::endl;
        return FALSE;
    }
    return TRUE;
}

static BOOLEAN
TestFloatingPointIr()
{
    if (sizeof(SYMBOL) != sizeof(UINT64) * 3 ||
        FUNC_MEMBER_ARROW_READ != 128 || FUNC_MOV_FLOAT != 129 || FUNC_NEG_FLOAT != 130 ||
        FUNC_ADD_FLOAT != 131 || FUNC_SUB_FLOAT != 132 || FUNC_MUL_FLOAT != 133 ||
        FUNC_DIV_FLOAT != 134 || FUNC_GT_FLOAT != 135 || FUNC_LT_FLOAT != 136 ||
        FUNC_EGT_FLOAT != 137 || FUNC_ELT_FLOAT != 138 || FUNC_EQUAL_FLOAT != 139 ||
        FUNC_NEQ_FLOAT != 140 || FUNC_CONVERT_FLOAT != 141)
    {
        return FALSE;
    }

    CHAR Script[] = "{ float single = 11.5; double wide = 0.789; double negativeZero = -0.0; }";
    PSYMBOL_BUFFER Buffer = (PSYMBOL_BUFFER)ScriptEngineParse(Script);
    if (!Buffer || Buffer->Message)
    {
        if (Buffer)
        {
            RemoveSymbolBuffer(Buffer);
        }
        return FALSE;
    }

    UINT32 MoveCount = 0;
    BOOLEAN FloatBitsFound = FALSE;
    BOOLEAN DoubleBitsFound = FALSE;
    BOOLEAN NegativeZeroFound = FALSE;

    for (UINT32 Index = 0; Index + 2 < Buffer->Pointer; Index++)
    {
        PSYMBOL Operator = Buffer->Head + Index;
        if (Operator->Type != SYMBOL_SEMANTIC_RULE_TYPE || Operator->Value != FUNC_MOV_FLOAT)
        {
            continue;
        }

        PSYMBOL Source = Buffer->Head + Index + 1;
        PSYMBOL Destination = Buffer->Head + Index + 2;
        MoveCount++;

        if (Source->Len != Destination->Len)
        {
            RemoveSymbolBuffer(Buffer);
            return FALSE;
        }

        FloatBitsFound |= Source->Len == SYMBOL_VALUE_KIND_FLOAT32 && Source->Value == 0x41380000ULL;
        DoubleBitsFound |= Source->Len == SYMBOL_VALUE_KIND_FLOAT64 && Source->Value == 0x3fe93f7ced916873ULL;
        NegativeZeroFound |= Source->Len == SYMBOL_VALUE_KIND_FLOAT64 && Source->Value == 0x8000000000000000ULL;
    }

    RemoveSymbolBuffer(Buffer);
    return MoveCount == 3 && FloatBitsFound && DoubleBitsFound && NegativeZeroFound;
}

static BOOLEAN
TestFloatingPointArithmeticIr()
{
    CHAR Script[] =
        "{ float leftValue = 1.5; float rightValue = 0.5; "
        "float addResult = leftValue + rightValue; float subResult = leftValue - rightValue; "
        "float mulResult = leftValue * rightValue; float divResult = leftValue / rightValue; "
        "if (addResult > subResult && addResult >= subResult && subResult < addResult && "
        "subResult <= addResult && addResult == addResult && addResult != subResult) { printf(\"ok\\n\"); } }";
    PSYMBOL_BUFFER Buffer = (PSYMBOL_BUFFER)ScriptEngineParse(Script);
    if (!Buffer || Buffer->Message)
    {
        if (Buffer) RemoveSymbolBuffer(Buffer);
        return FALSE;
    }

    BOOLEAN Seen[10] = {0};
    for (UINT32 Index = 0; Index < Buffer->Pointer; Index++)
    {
        PSYMBOL Symbol = Buffer->Head + Index;
        if (Symbol->Type != SYMBOL_SEMANTIC_RULE_TYPE ||
            Symbol->Value < FUNC_ADD_FLOAT || Symbol->Value > FUNC_NEQ_FLOAT)
        {
            continue;
        }

        UINT32 OpcodeIndex = (UINT32)(Symbol->Value - FUNC_ADD_FLOAT);
        Seen[OpcodeIndex] = TRUE;
        if (Index + 3 >= Buffer->Pointer ||
            Buffer->Head[Index + 1].Len != SYMBOL_VALUE_KIND_FLOAT32 ||
            Buffer->Head[Index + 2].Len != SYMBOL_VALUE_KIND_FLOAT32)
        {
            RemoveSymbolBuffer(Buffer);
            return FALSE;
        }

        BOOLEAN IsComparison = Symbol->Value >= FUNC_GT_FLOAT;
        if (Buffer->Head[Index + 3].Len !=
            (IsComparison ? SYMBOL_VALUE_KIND_INTEGER : SYMBOL_VALUE_KIND_FLOAT32))
        {
            RemoveSymbolBuffer(Buffer);
            return FALSE;
        }
    }

    RemoveSymbolBuffer(Buffer);
    for (UINT32 Index = 0; Index < 10; Index++)
    {
        if (!Seen[Index]) return FALSE;
    }
    return TRUE;
}

static BOOLEAN
TestFloatingPointDifferentialCases()
{
    struct OPERANDS
    {
        double Left;
        double Right;
    } Cases[] = {
        {0.1, 0.2}, {1.5, 0.5}, {-4.25, 2.5}, {7.0, 3.0},
        {31.75, -0.125}, {-9.5, -2.25}, {0.000125, 64.0}, {999.0, 0.75}};
    const CHAR Operators[] = {'+', '-', '*', '/'};

    for (const auto & Case : Cases)
    {
        for (CHAR Operator : Operators)
        {
            volatile double DoubleLeft  = Case.Left;
            volatile double DoubleRight = Case.Right;
            double DoubleExpected;
            if (Operator == '+') DoubleExpected = DoubleLeft + DoubleRight;
            else if (Operator == '-') DoubleExpected = DoubleLeft - DoubleRight;
            else if (Operator == '*') DoubleExpected = DoubleLeft * DoubleRight;
            else DoubleExpected = DoubleLeft / DoubleRight;

            CHAR Script[1024];
            _snprintf_s(Script,
                        sizeof(Script),
                        _TRUNCATE,
                        "{ double leftValue = %.20f; double rightValue = %.20f; "
                        "double expectedValue = %.20f; double actualValue = leftValue %c rightValue; "
                        "if (actualValue == expectedValue) { printf(\"ok\\n\"); } }",
                        Case.Left,
                        Case.Right,
                        DoubleExpected,
                        Operator);
            if (!RunScriptAndExpect(Script, TRUE, "ok\n")) return FALSE;

            volatile float FloatLeft  = (float)Case.Left;
            volatile float FloatRight = (float)Case.Right;
            float FloatExpected;
            if (Operator == '+') FloatExpected = FloatLeft + FloatRight;
            else if (Operator == '-') FloatExpected = FloatLeft - FloatRight;
            else if (Operator == '*') FloatExpected = FloatLeft * FloatRight;
            else FloatExpected = FloatLeft / FloatRight;

            _snprintf_s(Script,
                        sizeof(Script),
                        _TRUNCATE,
                        "{ float leftValue = %.20f; float rightValue = %.20f; "
                        "float expectedValue = %.20f; float actualValue = leftValue %c rightValue; "
                        "if (actualValue == expectedValue) { printf(\"ok\\n\"); } }",
                        (double)FloatLeft,
                        (double)FloatRight,
                        (double)FloatExpected,
                        Operator);
            if (!RunScriptAndExpect(Script, TRUE, "ok\n")) return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
TestScriptEngineFloatingPoint()
{
    const CHAR * ValidScript =
        "{ float varA = 11.5; float varB = .5; double varC = 0.789; double varZ = -0.0; double varP = +.5; "
        "printf(\"%f %f %f %f %f\\n\", varA, varB, varC, varZ, varP); printf(\"%f\\n\", -varC); }";

    if (!RunScriptAndExpect(ValidScript,
                            TRUE,
                            "11.500000 0.500000 0.789000 -0.000000 0.500000\n-0.789000\n"))
    {
        return FALSE;
    }

    if (!TestFloatingPointIr() || !TestFloatingPointArithmeticIr() ||
        !TestFloatingPointDifferentialCases() ||
        !RunScriptAndExpect("{ float var1 = 11.5; float var2 = 0.5; float result = var1 + var2; if (result == 12.0) { printf(\"arithmetic ok\\n\"); } }",
                            TRUE,
                            "arithmetic ok\n") ||
        !RunScriptAndExpect("{ double leftValue = 9.0; double rightValue = 4.0; double result = leftValue / rightValue; if (result >= 2.25 && result <= 2.25) { printf(\"comparison ok\\n\"); } }",
                            TRUE,
                            "comparison ok\n") ||
        !RunScriptAndExpect("{ float pointOne = 0.1; float pointTwo = 0.2; float expectedFloat = 0.3; float roundedFloat = pointOne + pointTwo; double third = 1.0 / 3.0; double expectedThird = 0.33333333333333331; double largeValue = 9007199254740992.0; double oneValue = 1.0; double roundedLarge = largeValue + oneValue; double negativeValue = -4.0; double positiveValue = 2.0; double negativeZeroValue = -0.0 * positiveValue; if (roundedFloat == expectedFloat && third == expectedThird && roundedLarge == largeValue && negativeValue < positiveValue && negativeValue <= positiveValue && positiveValue > negativeValue && positiveValue >= negativeValue && negativeZeroValue == 0.0) { printf(\"rounding and comparison ok %f\\n\", negativeZeroValue); } }",
                            TRUE,
                            "rounding and comparison ok -0.000000\n") ||
        !RunScriptAndExpect("{ float tinyValue = 0.000000000000000000000000000000000000000000001401298464324817; float expectedTinySum = 0.000000000000000000000000000000000000000000002802596928649634; float tinySum = tinyValue + tinyValue; if (tinySum == expectedTinySum) { printf(\"subnormal ok\\n\"); } }",
                            TRUE,
                            "subnormal ok\n") ||
        !RunScriptAndExpect("{ printf(\"%f\", 1); }", FALSE, NULL) ||
        !RunScriptAndExpect("{ double value = 1.5; printf(\"%x\", value); }", FALSE, NULL) ||
        !RunScriptAndExpect("{ double value = 1..2; }", FALSE, NULL) ||
        !RunScriptAndExpect("{ double value = 0x1.2; }", FALSE, NULL) ||
        !RunScriptAndExpect("{ double value = 1.0f; }", FALSE, NULL) ||
        !RunScriptAndExpect("{ float value = 999999999999999999999999999999999999999.0; }", FALSE, NULL) ||
        !RunScriptAndExpect("{ float value = 0.000000000000000000000000000000000000000000000000001; }", FALSE, NULL) ||
        !RunScriptAndExpectRuntimeFailure("{ double leftValue = 1.0; double zeroValue = 0.0; double result = leftValue / zeroValue; }") ||
        !RunScriptAndExpectRuntimeFailure("{ double largeValue = 99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999.0; double result = largeValue * largeValue; }"))
    {
        return FALSE;
    }

    return TRUE;
}
