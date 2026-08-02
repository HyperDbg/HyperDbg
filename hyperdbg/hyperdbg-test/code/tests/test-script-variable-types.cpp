/**
 * @file test-script-variable-types.cpp
 * @brief Focused parser, IR, and evaluator tests for scalar variable types.
 */
#include "pch.h"

static std::string CapturedVariableTypeOutput;

static VOID
CaptureVariableTypeOutput(CHAR * Message)
{
    if (Message)
        CapturedVariableTypeOutput.append(Message);
}

static BOOLEAN
RunVariableTypeScript(const CHAR * Script, const CHAR * ExpectedOutput)
{
    CapturedVariableTypeOutput.clear();
    hyperdbg_u_set_text_message_callback((PVOID)CaptureVariableTypeOutput);
    BOOLEAN Result = hyperdbg_u_test_script_engine((CHAR *)Script);
    hyperdbg_u_unset_text_message_callback();
    if (!Result || CapturedVariableTypeOutput != ExpectedOutput)
    {
        std::cerr << "Variable-type script failed: " << Script
                  << "\nExpected: " << ExpectedOutput
                  << "\nActual: " << CapturedVariableTypeOutput << std::endl;
        PSYMBOL_BUFFER Buffer = (PSYMBOL_BUFFER)ScriptEngineParse((CHAR *)Script);
        if (Buffer && !Buffer->Message)
        {
            for (UINT32 Index = 0; Index < Buffer->Pointer; Index++)
            {
                std::cerr << Index << ": type=" << Buffer->Head[Index].Type
                          << " len=" << Buffer->Head[Index].Len
                          << " value=" << Buffer->Head[Index].Value << std::endl;
            }
        }
        if (Buffer) RemoveSymbolBuffer(Buffer);
        return FALSE;
    }
    return TRUE;
}

static BOOLEAN
RunVariableTypeScriptExpectFailure(const CHAR * Script)
{
    CapturedVariableTypeOutput.clear();
    hyperdbg_u_set_text_message_callback((PVOID)CaptureVariableTypeOutput);
    BOOLEAN Result = hyperdbg_u_test_script_engine((CHAR *)Script);
    hyperdbg_u_unset_text_message_callback();
    if (Result)
    {
        std::cerr << "Variable-type script unexpectedly succeeded: " << Script << std::endl;
        return FALSE;
    }
    return TRUE;
}

static BOOLEAN
TestVariableTypeIrContract()
{
    if (sizeof(SYMBOL) != sizeof(UINT64) * 3 ||
        FUNC_CONVERT_FLOAT != 141 || FUNC_CAST_SCALAR != 142 ||
        FUNC_ADD_TYPED != 143 || FUNC_NEQ_TYPED != 158 ||
        FUNC_NEG_TYPED != 159 || FUNC_BITWISE_NOT_TYPED != 160 ||
        FUNC_LOGICAL_NOT_TYPED != 161 || FUNC_POINTER_DIFF != 162 ||
        SCRIPT_SCALAR_TYPE_INVALID != 0 || SCRIPT_SCALAR_TYPE_BOOL != 1 ||
        SCRIPT_SCALAR_TYPE_I8 != 2 || SCRIPT_SCALAR_TYPE_I16 != 3 ||
        SCRIPT_SCALAR_TYPE_I32 != 4 || SCRIPT_SCALAR_TYPE_I64 != 5 ||
        SCRIPT_SCALAR_TYPE_U8 != 6 || SCRIPT_SCALAR_TYPE_U16 != 7 ||
        SCRIPT_SCALAR_TYPE_U32 != 8 || SCRIPT_SCALAR_TYPE_U64 != 9 ||
        SCRIPT_SCALAR_TYPE_F32 != 10 || SCRIPT_SCALAR_TYPE_F64 != 11 ||
        SCRIPT_SCALAR_TYPE_POINTER != 12 || SCRIPT_SCALAR_TYPE_F80 != 13)
    {
        std::cerr << "Variable-type serialized IR constants changed unexpectedly" << std::endl;
        return FALSE;
    }

    CHAR Script[] = "{ unsigned char narrowValue = 0x12345; int value = narrowValue + 1; value %= 4; }";
    PSYMBOL_BUFFER Buffer = (PSYMBOL_BUFFER)ScriptEngineParse(Script);
    if (!Buffer || Buffer->Message)
    {
        std::cerr << "Variable-type IR script did not parse: "
                  << (Buffer && Buffer->Message ? Buffer->Message : "no buffer") << std::endl;
        if (Buffer) RemoveSymbolBuffer(Buffer);
        return FALSE;
    }

    BOOLEAN SawCast = FALSE;
    BOOLEAN SawAdd = FALSE;
    BOOLEAN SawModulo = FALSE;
    for (UINT32 Index = 0; Index < Buffer->Pointer; Index++)
    {
        if (Buffer->Head[Index].Type != SYMBOL_SEMANTIC_RULE_TYPE)
            continue;
        SawCast |= Buffer->Head[Index].Value == FUNC_CAST_SCALAR;
        SawAdd |= Buffer->Head[Index].Value == FUNC_ADD_TYPED;
        SawModulo |= Buffer->Head[Index].Value == FUNC_MOD_TYPED;
    }
    RemoveSymbolBuffer(Buffer);
    if (!SawCast || !SawAdd || !SawModulo)
    {
        std::cerr << "Variable-type IR is missing cast/add/modulo: "
                  << SawCast << "/" << SawAdd << "/" << SawModulo << std::endl;
    }
    return SawCast && SawAdd && SawModulo;
}

BOOLEAN
TestScriptEngineVariableTypes()
{
    return TestVariableTypeIrContract() &&
           RunVariableTypeScript("{ char narrow = 0xff; unsigned char narrowValue = 0x12345; int promoted = narrowValue; printf(\"%lld %lld %lld\\n\", narrow, narrowValue, promoted); }",
                                 "-1 69 69\n") &&
           RunVariableTypeScript("{ int value = 0n10; value %= 4; long wide = 0n10; wide %= 4; printf(\"%lld %lld\\n\", value, wide); }",
                                 "2 2\n") &&
           RunVariableTypeScript("{ printf(\"%lld %lld %lld %lld %lld\\n\", sizeof(char), sizeof(short), sizeof(int), sizeof(long), sizeof(long long)); }",
                                 "1 2 4 8 8\n") &&
           RunVariableTypeScript("{ printf(\"%lld %lld\\n\", sizeof(!(char)0), sizeof(!(long)0)); }",
                                 "4 4\n") &&
           RunVariableTypeScript("{ if (sizeof(char) == 1 && (int)1 == 1) { printf(\"boolean type syntax\\n\"); } }",
                                 "boolean type syntax\n") &&
           RunVariableTypeScript("{ struct Pair { int left; unsigned short right; }; printf(\"%lld\\n\", sizeof(struct Pair)); }",
                                 "8\n") &&
           RunVariableTypeScript("{ struct NarrowFields { char signedValue; unsigned char unsignedValue; }; struct NarrowFields fields; fields.signedValue = 0xff; fields.unsignedValue = 0x1ff; printf(\"%lld %lld\\n\", fields.signedValue, fields.unsignedValue); }",
                                 "-1 255\n") &&
           RunVariableTypeScript("{ struct CompoundFields { int signedValue; unsigned char unsignedValue; }; struct CompoundFields fields; fields.signedValue = 0n10; fields.signedValue %= 4; fields.unsignedValue = 0x105; fields.unsignedValue += 0x100; printf(\"%lld %lld\\n\", fields.signedValue, fields.unsignedValue); }",
                                 "2 5\n") &&
           RunVariableTypeScript("{ char values[2] = {0xff, 0x7f}; printf(\"%lld %lld\\n\", values[0], values[1]); }",
                                 "-1 127\n") &&
           RunVariableTypeScript("{ typedef unsigned short word; word value = 0x12345; printf(\"%lld %lld\\n\", value, sizeof(word)); }",
                                 "9029 2\n") &&
           RunVariableTypeScript("{ int probe() { printf(\"evaluated\\n\"); return 1; } printf(\"%lld\\n\", sizeof(probe())); }",
                                 "4\n") &&
           RunVariableTypeScript("{ int add_to_55(int value) { return 0n55 + value; } result = add_to_55(0n47); printf(\"%lld\\n\", result); }",
                                 "102\n") &&
           RunVariableTypeScript("{ int typed_factorial(int value) { if (value == 0 || value == 1) { return 1; } return value * typed_factorial(value - 1); } result = typed_factorial(0n10); printf(\"%lld\\n\", result); }",
                                 "3628800\n") &&
           RunVariableTypeScript("{ source_value = 0n123456; destination_value = 0; memcpy(&destination_value, &source_value, 8); printf(\"%lld\\n\", destination_value); }",
                                 "123456\n") &&
           RunVariableTypeScript("{ local_value = 0n55; result = &local_value; printf(\"%lld %lld\\n\", sizeof(result), dq(result)); }",
                                 "8 55\n") &&
           RunVariableTypeScript("{ .global_value = 0n55; result = &(.global_value); printf(\"%lld %lld\\n\", sizeof(result), dq(result)); }",
                                 "8 55\n") &&
           RunVariableTypeScript("{ char narrow_value = 0x7f; char *narrow_pointer = &narrow_value; printf(\"%lld\\n\", *narrow_pointer); }",
                                 "127\n") &&
           RunVariableTypeScript("{ int integerValue = (int)(unsigned char)0x12345; double doubleValue = (double)integerValue; long restored = (long)doubleValue; printf(\"%lld %f %lld\\n\", integerValue, doubleValue, restored); }",
                                 "69 69.000000 69\n") &&
           RunVariableTypeScript("{ char *left = (char *)0xffffffffffffffff; char *right = (char *)0xfffffffffffffff0; if (left > right) { printf(\"%lld %lld 1\\n\", left - right, right - left); } }",
                                 "15 -15 1\n") &&
           RunVariableTypeScript("{ char *base = (char *)0x1000; char *next = base + 0n2; printf(\"%lld\\n\", next - base); }",
                                 "2\n") &&
           RunVariableTypeScript("{ if (1 || 1 / 0) { printf(\"or short circuit\\n\"); } if (0 && 1 / 0) { printf(\"unexpected\\n\"); } printf(\"and short circuit\\n\"); }",
                                 "or short circuit\nand short circuit\n") &&
           RunVariableTypeScript("{ inferred = 0xffffffffffffffff; printf(\"%llu %lld\\n\", inferred, sizeof(inferred)); }",
                                 "18446744073709551615 8\n") &&
           RunVariableTypeScriptExpectFailure("{ unsigned float invalidValue = 1.0; }") &&
           RunVariableTypeScriptExpectFailure("{ long double invalidValue = 1.0; }") &&
           RunVariableTypeScriptExpectFailure("{ long long long invalidValue = 1; }") &&
           RunVariableTypeScriptExpectFailure("{ local_value = 0n55; unsigned long long explicit_result = &local_value; }") &&
           RunVariableTypeScriptExpectFailure("{ int value = 1; value = value / 0; }") &&
           RunVariableTypeScriptExpectFailure("{ unsigned long value = 1; value = value << 0n64; }");
}
