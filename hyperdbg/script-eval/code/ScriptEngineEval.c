/**
 * @file ScriptEngineEval.c
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Shared Headers for Script engine
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"
#include "../script-eval/header/ScriptEngineInternalHeader.h"

static BOOLEAN
ScriptEngineTypedLocalRangeIsValid(PSCRIPT_ENGINE_GENERAL_REGISTERS Registers, UINT64 Address, UINT32 Size)
{
    UINT64 Begin = (UINT64)Registers->StackBuffer;
    UINT64 End   = Begin + (MAX_STACK_BUFFER_COUNT * sizeof(UINT64));
    return Size && Address >= Begin && Address <= End && Size <= End - Address;
}

static BOOLEAN
ScriptEngineTransferMemory(PSCRIPT_ENGINE_GENERAL_REGISTERS Registers,
                          UINT64 Address,
                          UINT64 AddressSpace,
                          PVOID Buffer,
                          UINT32 Size,
                          BOOLEAN Write)
{
    if (AddressSpace == SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL)
    {
        if (!ScriptEngineTypedLocalRangeIsValid(Registers, Address, Size))
            return FALSE;
        if (Write)
            memcpy((PVOID)Address, Buffer, Size);
        else
            memcpy(Buffer, (PVOID)Address, Size);
        return TRUE;
    }

    if (AddressSpace != SCRIPT_ENGINE_ADDRESS_SPACE_REMOTE ||
        !CheckAccessValidityAndSafety(Address, Size))
        return FALSE;

#ifdef SCRIPT_ENGINE_USER_MODE
    if (Write)
        memcpy((PVOID)Address, Buffer, Size);
    else
        memcpy(Buffer, (PVOID)Address, Size);
#endif
#ifdef SCRIPT_ENGINE_KERNEL_MODE
    if (Write)
        MemoryMapperWriteMemorySafeOnTargetProcess(Address, Buffer, Size);
    else
        MemoryMapperReadMemorySafeOnTargetProcess(Address, Buffer, Size);
#endif
    return TRUE;
}

static BOOLEAN
ScriptEngineFloatingSymbolIsReadable(PSCRIPT_ENGINE_GENERAL_REGISTERS Registers, PSYMBOL Symbol)
{
    UINT64 BaseType = Symbol->Type & 0xffffffffULL;

    if (Symbol->Len != SYMBOL_VALUE_KIND_FLOAT32 && Symbol->Len != SYMBOL_VALUE_KIND_FLOAT64)
    {
        return FALSE;
    }

    if (BaseType == SYMBOL_NUM_TYPE)
    {
        return TRUE;
    }

    return BaseType == SYMBOL_TEMP_TYPE &&
           Registers->StackBaseIndx < MAX_STACK_BUFFER_COUNT &&
           Symbol->Value < MAX_STACK_BUFFER_COUNT - Registers->StackBaseIndx;
}

static BOOLEAN
ScriptEngineFloatingSymbolIsWritable(PSCRIPT_ENGINE_GENERAL_REGISTERS Registers, PSYMBOL Symbol)
{
    UINT64 BaseType = Symbol->Type & 0xffffffffULL;

    return (Symbol->Len == SYMBOL_VALUE_KIND_FLOAT32 || Symbol->Len == SYMBOL_VALUE_KIND_FLOAT64) &&
           BaseType == SYMBOL_TEMP_TYPE &&
           Registers->StackBaseIndx < MAX_STACK_BUFFER_COUNT &&
           Symbol->Value < MAX_STACK_BUFFER_COUNT - Registers->StackBaseIndx;
}

typedef struct _SCRIPT_ENGINE_UINT128
{
    UINT64 High;
    UINT64 Low;
} SCRIPT_ENGINE_UINT128, *PSCRIPT_ENGINE_UINT128;

//
// The software-float path represents a finite value as a sign, an unbiased
// exponent, and a normalized 128-bit significand whose leading bit is bit 127.
// Guard, round, and sticky bits remain in the low portion until packing.
//
// Keep all 128-bit transfers as explicit scalar field operations. MSVC may
// otherwise turn structure copies or zero initializers into XMM instructions
// in kernel builds, even though the source contains no C floating-point types.
//

typedef struct _SCRIPT_ENGINE_SOFT_FLOAT
{
    BOOLEAN               Negative;
    BOOLEAN               IsZero;
    INT32                 Exponent;
    SCRIPT_ENGINE_UINT128 Significand;
} SCRIPT_ENGINE_SOFT_FLOAT;

static BOOLEAN
ScriptEngineSoftFloatUnpack(UINT64 ValueKind, UINT64 RawBits, SCRIPT_ENGINE_SOFT_FLOAT * Value);

static BOOLEAN
ScriptEngineSoftFloatPack(UINT64 ValueKind,
                          BOOLEAN Negative,
                          INT32 Exponent,
                          const SCRIPT_ENGINE_UINT128 * InputSignificand,
                          PUINT64 RawBits);

static BOOLEAN
ScriptEngineIntegerSymbolIsWritable(PSCRIPT_ENGINE_GENERAL_REGISTERS Registers, PSYMBOL Symbol)
{
    return Symbol->Len == SYMBOL_VALUE_KIND_INTEGER &&
           (Symbol->Type & 0xffffffffULL) == SYMBOL_TEMP_TYPE &&
           Registers->StackBaseIndx < MAX_STACK_BUFFER_COUNT &&
           Symbol->Value < MAX_STACK_BUFFER_COUNT - Registers->StackBaseIndx;
}

static BOOLEAN
ScriptEngineHasOperands(PSYMBOL_BUFFER CodeBuffer, UINT64 Index, UINT32 Count)
{
    return Index <= CodeBuffer->Pointer && Count <= CodeBuffer->Pointer - Index;
}

static BOOLEAN
ScriptEngineScalarTypeIsInteger(UINT64 TypeId)
{
    return TypeId >= SCRIPT_SCALAR_TYPE_BOOL && TypeId <= SCRIPT_SCALAR_TYPE_U64;
}

static BOOLEAN
ScriptEngineScalarTypeIsFloating(UINT64 TypeId)
{
    return TypeId == SCRIPT_SCALAR_TYPE_F32 || TypeId == SCRIPT_SCALAR_TYPE_F64;
}

static UINT32
ScriptEngineScalarTypeWidth(UINT64 TypeId)
{
    switch (TypeId)
    {
    case SCRIPT_SCALAR_TYPE_BOOL:
    case SCRIPT_SCALAR_TYPE_I8:
    case SCRIPT_SCALAR_TYPE_U8:
        return 8;
    case SCRIPT_SCALAR_TYPE_I16:
    case SCRIPT_SCALAR_TYPE_U16:
        return 16;
    case SCRIPT_SCALAR_TYPE_I32:
    case SCRIPT_SCALAR_TYPE_U32:
    case SCRIPT_SCALAR_TYPE_F32:
        return 32;
    case SCRIPT_SCALAR_TYPE_I64:
    case SCRIPT_SCALAR_TYPE_U64:
    case SCRIPT_SCALAR_TYPE_F64:
    case SCRIPT_SCALAR_TYPE_POINTER:
        return 64;
    default:
        return 0;
    }
}

static BOOLEAN
ScriptEngineScalarTypeIsSigned(UINT64 TypeId)
{
    return TypeId == SCRIPT_SCALAR_TYPE_I8 || TypeId == SCRIPT_SCALAR_TYPE_I16 ||
           TypeId == SCRIPT_SCALAR_TYPE_I32 || TypeId == SCRIPT_SCALAR_TYPE_I64;
}

static UINT64
ScriptEngineNormalizeInteger(UINT64 Value, UINT64 TypeId)
{
    UINT32 Width = ScriptEngineScalarTypeWidth(TypeId);
    UINT64 Mask;

    if (TypeId == SCRIPT_SCALAR_TYPE_BOOL)
        return Value != 0;
    if (Width == 64)
        return Value;
    Mask  = (1ULL << Width) - 1;
    Value &= Mask;
    if (ScriptEngineScalarTypeIsSigned(TypeId) && (Value & (1ULL << (Width - 1))))
        Value |= ~Mask;
    return Value;
}

static BOOLEAN
ScriptEngineIntegerToFloat(UINT64 Value, UINT64 SourceType, UINT64 DestinationType, PUINT64 Result)
{
    SCRIPT_ENGINE_UINT128 Significand = {0, 0};
    BOOLEAN Negative = FALSE;
    UINT64 Magnitude;
    INT32 Exponent = 0;

    Value = ScriptEngineNormalizeInteger(Value, SourceType);
    if (ScriptEngineScalarTypeIsSigned(SourceType) && (Value & 0x8000000000000000ULL))
    {
        Negative  = TRUE;
        Magnitude = 0ULL - Value;
    }
    else
    {
        Magnitude = Value;
    }

    if (Magnitude)
    {
        UINT64 Scan = Magnitude;
        while (Scan >>= 1) Exponent++;
        Significand.High = Magnitude << (63 - Exponent);
    }

    return ScriptEngineSoftFloatPack(DestinationType == SCRIPT_SCALAR_TYPE_F32 ?
                                         SYMBOL_VALUE_KIND_FLOAT32 : SYMBOL_VALUE_KIND_FLOAT64,
                                     Negative,
                                     Exponent,
                                     &Significand,
                                     Result);
}

static BOOLEAN
ScriptEngineFloatToInteger(UINT64 Value, UINT64 SourceType, UINT64 DestinationType, PUINT64 Result)
{
    SCRIPT_ENGINE_SOFT_FLOAT FloatValue;
    UINT64 Magnitude;
    UINT64 Maximum;
    UINT32 Width = ScriptEngineScalarTypeWidth(DestinationType);

    if (!ScriptEngineSoftFloatUnpack(SourceType == SCRIPT_SCALAR_TYPE_F32 ?
                                         SYMBOL_VALUE_KIND_FLOAT32 : SYMBOL_VALUE_KIND_FLOAT64,
                                     Value,
                                     &FloatValue))
        return FALSE;
    if (FloatValue.IsZero || FloatValue.Exponent < 0)
    {
        *Result = 0;
        return TRUE;
    }
    if (FloatValue.Exponent > 63)
        return FALSE;

    Magnitude = FloatValue.Significand.High >> (63 - FloatValue.Exponent);
    if (ScriptEngineScalarTypeIsSigned(DestinationType))
    {
        Maximum = Width == 64 ? 0x7fffffffffffffffULL : (1ULL << (Width - 1)) - 1;
        if ((!FloatValue.Negative && Magnitude > Maximum) ||
            (FloatValue.Negative && Magnitude > Maximum + 1))
            return FALSE;
        *Result = FloatValue.Negative ? 0ULL - Magnitude : Magnitude;
    }
    else
    {
        Maximum = Width == 64 ? ~0ULL : (1ULL << Width) - 1;
        if (FloatValue.Negative || Magnitude > Maximum)
            return FALSE;
        *Result = Magnitude;
    }
    *Result = ScriptEngineNormalizeInteger(*Result, DestinationType);
    return TRUE;
}

static BOOLEAN
ScriptEngineCastScalar(UINT64 Value, UINT64 SourceType, UINT64 DestinationType, PUINT64 Result)
{
    if (SourceType == SCRIPT_SCALAR_TYPE_F80 || DestinationType == SCRIPT_SCALAR_TYPE_F80 ||
        SourceType == SCRIPT_SCALAR_TYPE_INVALID || DestinationType == SCRIPT_SCALAR_TYPE_INVALID)
        return FALSE;

    if ((ScriptEngineScalarTypeIsInteger(SourceType) || SourceType == SCRIPT_SCALAR_TYPE_POINTER) &&
        (ScriptEngineScalarTypeIsInteger(DestinationType) || DestinationType == SCRIPT_SCALAR_TYPE_POINTER))
    {
        *Result = DestinationType == SCRIPT_SCALAR_TYPE_POINTER ?
                      Value : ScriptEngineNormalizeInteger(Value, DestinationType);
        return TRUE;
    }
    if (ScriptEngineScalarTypeIsFloating(SourceType) && ScriptEngineScalarTypeIsFloating(DestinationType))
    {
        SCRIPT_ENGINE_SOFT_FLOAT Converted;
        if (!ScriptEngineSoftFloatUnpack(SourceType == SCRIPT_SCALAR_TYPE_F32 ?
                                             SYMBOL_VALUE_KIND_FLOAT32 : SYMBOL_VALUE_KIND_FLOAT64,
                                         Value,
                                         &Converted))
            return FALSE;
        return ScriptEngineSoftFloatPack(DestinationType == SCRIPT_SCALAR_TYPE_F32 ?
                                             SYMBOL_VALUE_KIND_FLOAT32 : SYMBOL_VALUE_KIND_FLOAT64,
                                         Converted.Negative,
                                         Converted.Exponent,
                                         &Converted.Significand,
                                         Result);
    }
    if (ScriptEngineScalarTypeIsInteger(SourceType) && ScriptEngineScalarTypeIsFloating(DestinationType))
        return ScriptEngineIntegerToFloat(Value, SourceType, DestinationType, Result);
    if (ScriptEngineScalarTypeIsFloating(SourceType) && ScriptEngineScalarTypeIsInteger(DestinationType))
        return ScriptEngineFloatToInteger(Value, SourceType, DestinationType, Result);
    return FALSE;
}

static BOOLEAN
ScriptEngineExecuteTypedBinary(UINT64 Opcode,
                               UINT64 Right,
                               UINT64 Left,
                               UINT64 TypeId,
                               PUINT64 Result)
{
    UINT32 Width;
    UINT64 SignedMinimum;

    if (TypeId == SCRIPT_SCALAR_TYPE_POINTER)
    {
        if (Opcode == FUNC_GT_TYPED) *Result = Left > Right;
        else if (Opcode == FUNC_LT_TYPED) *Result = Left < Right;
        else if (Opcode == FUNC_EGT_TYPED) *Result = Left >= Right;
        else if (Opcode == FUNC_ELT_TYPED) *Result = Left <= Right;
        else if (Opcode == FUNC_EQUAL_TYPED) *Result = Left == Right;
        else if (Opcode == FUNC_NEQ_TYPED) *Result = Left != Right;
        else return FALSE;
        return TRUE;
    }
    if (!ScriptEngineScalarTypeIsInteger(TypeId))
        return FALSE;
    Width = ScriptEngineScalarTypeWidth(TypeId);
    Left  = ScriptEngineNormalizeInteger(Left, TypeId);
    Right = ScriptEngineNormalizeInteger(Right, TypeId);

    switch (Opcode)
    {
    case FUNC_ADD_TYPED: *Result = Left + Right; break;
    case FUNC_SUB_TYPED: *Result = Left - Right; break;
    case FUNC_MUL_TYPED: *Result = Left * Right; break;
    case FUNC_DIV_TYPED:
    case FUNC_MOD_TYPED:
        if (!Right) return FALSE;
        if (ScriptEngineScalarTypeIsSigned(TypeId))
        {
            SignedMinimum = Width == 64 ? 0x8000000000000000ULL : (1ULL << (Width - 1));
            SignedMinimum = ScriptEngineNormalizeInteger(SignedMinimum, TypeId);
            if (Left == SignedMinimum && Right == ~0ULL) return FALSE;
            *Result = Opcode == FUNC_DIV_TYPED ?
                          (UINT64)((INT64)Left / (INT64)Right) :
                          (UINT64)((INT64)Left % (INT64)Right);
        }
        else
        {
            *Result = Opcode == FUNC_DIV_TYPED ? Left / Right : Left % Right;
        }
        break;
    case FUNC_BITWISE_AND_TYPED: *Result = Left & Right; break;
    case FUNC_BITWISE_OR_TYPED: *Result = Left | Right; break;
    case FUNC_BITWISE_XOR_TYPED: *Result = Left ^ Right; break;
    case FUNC_SHIFT_LEFT_TYPED:
        if (Right >= Width) return FALSE;
        *Result = Left << Right;
        break;
    case FUNC_SHIFT_RIGHT_TYPED:
        if (Right >= Width) return FALSE;
        *Result = ScriptEngineScalarTypeIsSigned(TypeId) ?
                      (UINT64)((INT64)Left >> Right) : Left >> Right;
        break;
    case FUNC_GT_TYPED: *Result = ScriptEngineScalarTypeIsSigned(TypeId) ? (INT64)Left > (INT64)Right : Left > Right; return TRUE;
    case FUNC_LT_TYPED: *Result = ScriptEngineScalarTypeIsSigned(TypeId) ? (INT64)Left < (INT64)Right : Left < Right; return TRUE;
    case FUNC_EGT_TYPED: *Result = ScriptEngineScalarTypeIsSigned(TypeId) ? (INT64)Left >= (INT64)Right : Left >= Right; return TRUE;
    case FUNC_ELT_TYPED: *Result = ScriptEngineScalarTypeIsSigned(TypeId) ? (INT64)Left <= (INT64)Right : Left <= Right; return TRUE;
    case FUNC_EQUAL_TYPED: *Result = Left == Right; return TRUE;
    case FUNC_NEQ_TYPED: *Result = Left != Right; return TRUE;
    default: return FALSE;
    }

    *Result = ScriptEngineNormalizeInteger(*Result, TypeId);
    return TRUE;
}

static BOOLEAN
ScriptEngineIsFloatingComparisonOpcode(UINT64 Opcode)
{
    return Opcode == FUNC_GT_FLOAT || Opcode == FUNC_LT_FLOAT ||
           Opcode == FUNC_EGT_FLOAT || Opcode == FUNC_ELT_FLOAT ||
           Opcode == FUNC_EQUAL_FLOAT || Opcode == FUNC_NEQ_FLOAT;
}

static BOOLEAN
ScriptEngineUint128IsZero(const SCRIPT_ENGINE_UINT128 * Value)
{
    return Value->High == 0 && Value->Low == 0;
}

static INT32
ScriptEngineUint128Compare(const SCRIPT_ENGINE_UINT128 * Left, const SCRIPT_ENGINE_UINT128 * Right)
{
    if (Left->High != Right->High)
    {
        return Left->High > Right->High ? 1 : -1;
    }
    if (Left->Low != Right->Low)
    {
        return Left->Low > Right->Low ? 1 : -1;
    }
    return 0;
}

static VOID
ScriptEngineUint128ShiftRightJam(PSCRIPT_ENGINE_UINT128 Value, UINT32 Distance)
{
    UINT64  High = Value->High;
    UINT64  Low  = Value->Low;
    BOOLEAN Jam;

    if (Distance == 0)
    {
        return;
    }
    if (Distance < 64)
    {
        Jam         = (Low << (64 - Distance)) != 0;
        Value->High = High >> Distance;
        Value->Low  = (High << (64 - Distance)) | (Low >> Distance) | Jam;
    }
    else if (Distance == 64)
    {
        Value->High = 0;
        Value->Low  = High | (Low != 0);
    }
    else if (Distance < 128)
    {
        UINT32 Shift = Distance - 64;
        Jam          = Low != 0 || (High << (64 - Shift)) != 0;
        Value->High  = 0;
        Value->Low   = (High >> Shift) | Jam;
    }
    else
    {
        Value->High = 0;
        Value->Low  = (High != 0 || Low != 0);
    }
}

static VOID
ScriptEngineUint128ShiftLeftOne(PSCRIPT_ENGINE_UINT128 Value)
{
    Value->High = (Value->High << 1) | (Value->Low >> 63);
    Value->Low <<= 1;
}

static VOID
ScriptEngineUint128Subtract(const SCRIPT_ENGINE_UINT128 * Left,
                            const SCRIPT_ENGINE_UINT128 * Right,
                            PSCRIPT_ENGINE_UINT128 Result)
{
    Result->Low  = Left->Low - Right->Low;
    Result->High = Left->High - Right->High - (Left->Low < Right->Low);
}

static VOID
ScriptEngineUint128Add(const SCRIPT_ENGINE_UINT128 * Left,
                       const SCRIPT_ENGINE_UINT128 * Right,
                       PSCRIPT_ENGINE_UINT128 Result,
                       PBOOLEAN Carry)
{
    UINT64                HighWithoutCarry;
    BOOLEAN               LowCarry;
    BOOLEAN               HighCarry;

    Result->Low      = Left->Low + Right->Low;
    LowCarry         = Result->Low < Left->Low;
    HighWithoutCarry = Left->High + Right->High;
    HighCarry        = HighWithoutCarry < Left->High;
    Result->High     = HighWithoutCarry + LowCarry;
    *Carry           = HighCarry || (LowCarry && Result->High == 0);
}

static VOID
ScriptEngineUint128Multiply64(UINT64 Left, UINT64 Right, PSCRIPT_ENGINE_UINT128 Result)
{
    UINT64 LeftLow   = (UINT32)Left;
    UINT64 LeftHigh  = Left >> 32;
    UINT64 RightLow  = (UINT32)Right;
    UINT64 RightHigh = Right >> 32;
    UINT64 Product00 = LeftLow * RightLow;
    UINT64 Product01 = LeftLow * RightHigh;
    UINT64 Product10 = LeftHigh * RightLow;
    UINT64 Product11 = LeftHigh * RightHigh;
    UINT64 Middle    = (Product00 >> 32) + (UINT32)Product01 + (UINT32)Product10;
    Result->Low  = (Middle << 32) | (UINT32)Product00;
    Result->High = Product11 + (Product01 >> 32) + (Product10 >> 32) + (Middle >> 32);
}

static BOOLEAN
ScriptEngineSoftFloatUnpack(UINT64 ValueKind, UINT64 RawBits, SCRIPT_ENGINE_SOFT_FLOAT * Value)
{
    UINT64 Fraction;
    UINT32 Precision;
    INT32  MinimumExponent;
    UINT32 EncodedExponent;

    if (!Value)
    {
        return FALSE;
    }

    if (ValueKind == SYMBOL_VALUE_KIND_FLOAT32)
    {
        UINT32 Bits       = (UINT32)RawBits;
        Value->Negative   = (Bits >> 31) != 0;
        EncodedExponent   = (Bits >> 23) & 0xff;
        Fraction          = Bits & 0x7fffff;
        Precision         = 24;
        MinimumExponent   = -126;
        if (EncodedExponent == 0xff) return FALSE;
        Value->Exponent = EncodedExponent ? (INT32)EncodedExponent - 127 : MinimumExponent;
    }
    else if (ValueKind == SYMBOL_VALUE_KIND_FLOAT64)
    {
        Value->Negative   = (RawBits >> 63) != 0;
        EncodedExponent   = (UINT32)((RawBits >> 52) & 0x7ff);
        Fraction          = RawBits & 0xfffffffffffffULL;
        Precision         = 53;
        MinimumExponent   = -1022;
        if (EncodedExponent == 0x7ff) return FALSE;
        Value->Exponent = EncodedExponent ? (INT32)EncodedExponent - 1023 : MinimumExponent;
    }
    else
    {
        return FALSE;
    }

    Value->IsZero = EncodedExponent == 0 && Fraction == 0;
    if (Value->IsZero)
    {
        Value->Significand.High = 0;
        Value->Significand.Low  = 0;
        return TRUE;
    }

    if (EncodedExponent)
    {
        Fraction |= 1ULL << (Precision - 1);
    }
    else
    {
        while ((Fraction & (1ULL << (Precision - 1))) == 0)
        {
            Fraction <<= 1;
            Value->Exponent--;
        }
    }

    Value->Significand.High = Fraction << (64 - Precision);
    Value->Significand.Low  = 0;
    return TRUE;
}

static BOOLEAN
ScriptEngineSoftFloatPack(UINT64 ValueKind,
                          BOOLEAN Negative,
                          INT32 Exponent,
                          const SCRIPT_ENGINE_UINT128 * InputSignificand,
                          PUINT64 RawBits)
{
    SCRIPT_ENGINE_UINT128 Significand;
    UINT32 Precision;
    INT32  MinimumExponent;
    INT32  MaximumExponent;
    UINT32 Bias;
    UINT64 SignMask;
    UINT64 FractionMask;

    if (!RawBits || !InputSignificand) return FALSE;
    Significand.High = InputSignificand->High;
    Significand.Low  = InputSignificand->Low;
    if (ValueKind == SYMBOL_VALUE_KIND_FLOAT32)
    {
        Precision = 24; MinimumExponent = -126; MaximumExponent = 127;
        Bias = 127; SignMask = 0x80000000ULL; FractionMask = 0x7fffffULL;
    }
    else if (ValueKind == SYMBOL_VALUE_KIND_FLOAT64)
    {
        Precision = 53; MinimumExponent = -1022; MaximumExponent = 1023;
        Bias = 1023; SignMask = 0x8000000000000000ULL; FractionMask = 0xfffffffffffffULL;
    }
    else return FALSE;

    if (ScriptEngineUint128IsZero(&Significand))
    {
        *RawBits = Negative ? SignMask : 0;
        return TRUE;
    }

    while ((Significand.High & 0x8000000000000000ULL) == 0)
    {
        ScriptEngineUint128ShiftLeftOne(&Significand);
        Exponent--;
    }

    if (Exponent < MinimumExponent)
    {
        ScriptEngineUint128ShiftRightJam(&Significand, (UINT32)(MinimumExponent - Exponent));
        Exponent    = MinimumExponent;
    }

    UINT32 DiscardedHighBits = 64 - Precision;
    UINT64 Retained          = Significand.High >> DiscardedHighBits;
    UINT64 RoundMask         = 1ULL << (DiscardedHighBits - 1);
    BOOLEAN RoundBit         = (Significand.High & RoundMask) != 0;
    BOOLEAN Sticky           = (Significand.High & (RoundMask - 1)) != 0 || Significand.Low != 0;
    if (RoundBit && (Sticky || (Retained & 1)))
    {
        Retained++;
    }

    if (Retained == (1ULL << Precision))
    {
        Retained >>= 1;
        Exponent++;
    }
    if (Exponent > MaximumExponent)
    {
        return FALSE;
    }

    UINT64 HiddenBit       = 1ULL << (Precision - 1);
    UINT64 EncodedExponent = Retained >= HiddenBit ? (UINT64)(Exponent + (INT32)Bias) : 0;
    UINT64 Encoded         = (EncodedExponent << (Precision - 1)) | (Retained & FractionMask);
    *RawBits = Encoded | (Negative ? SignMask : 0);
    if (ValueKind == SYMBOL_VALUE_KIND_FLOAT32) *RawBits &= 0xffffffffULL;
    return TRUE;
}

static BOOLEAN
ScriptEngineSoftFloatAdd(const SCRIPT_ENGINE_SOFT_FLOAT * LeftValue,
                         const SCRIPT_ENGINE_SOFT_FLOAT * RightValue,
                         UINT64 ResultKind,
                         PUINT64 RawResult)
{
    SCRIPT_ENGINE_SOFT_FLOAT Left;
    SCRIPT_ENGINE_SOFT_FLOAT Right;
    Left.Negative = LeftValue->Negative; Left.IsZero = LeftValue->IsZero; Left.Exponent = LeftValue->Exponent;
    Left.Significand.High = LeftValue->Significand.High; Left.Significand.Low = LeftValue->Significand.Low;
    Right.Negative = RightValue->Negative; Right.IsZero = RightValue->IsZero; Right.Exponent = RightValue->Exponent;
    Right.Significand.High = RightValue->Significand.High; Right.Significand.Low = RightValue->Significand.Low;

    if (Left.IsZero && Right.IsZero)
    {
        SCRIPT_ENGINE_UINT128 Zero;
        Zero.High = 0;
        Zero.Low  = 0;
        return ScriptEngineSoftFloatPack(ResultKind,
                                         Left.Negative == Right.Negative ? Left.Negative : FALSE,
                                         0,
                                         &Zero,
                                         RawResult);
    }
    if (Left.IsZero) return ScriptEngineSoftFloatPack(ResultKind, Right.Negative, Right.Exponent, &Right.Significand, RawResult);
    if (Right.IsZero) return ScriptEngineSoftFloatPack(ResultKind, Left.Negative, Left.Exponent, &Left.Significand, RawResult);

    if (Right.Exponent > Left.Exponent)
    {
        SCRIPT_ENGINE_SOFT_FLOAT Swap;
        Swap.Negative = Left.Negative; Swap.IsZero = Left.IsZero; Swap.Exponent = Left.Exponent;
        Swap.Significand.High = Left.Significand.High; Swap.Significand.Low = Left.Significand.Low;
        Left.Negative = Right.Negative; Left.IsZero = Right.IsZero; Left.Exponent = Right.Exponent;
        Left.Significand.High = Right.Significand.High; Left.Significand.Low = Right.Significand.Low;
        Right.Negative = Swap.Negative; Right.IsZero = Swap.IsZero; Right.Exponent = Swap.Exponent;
        Right.Significand.High = Swap.Significand.High; Right.Significand.Low = Swap.Significand.Low;
    }
    ScriptEngineUint128ShiftRightJam(&Right.Significand,
                                     (UINT32)(Left.Exponent - Right.Exponent));

    SCRIPT_ENGINE_UINT128 Result;
    BOOLEAN               ResultSign;
    INT32                 ResultExponent = Left.Exponent;
    if (Left.Negative == Right.Negative)
    {
        BOOLEAN Carry;
        ScriptEngineUint128Add(&Left.Significand, &Right.Significand, &Result, &Carry);
        if (Carry)
        {
            BOOLEAN Lost = (Result.Low & 1) != 0;
            Result.Low   = (Result.High << 63) | (Result.Low >> 1) | Lost;
            Result.High  = 0x8000000000000000ULL | (Result.High >> 1);
            ResultExponent++;
        }
        ResultSign = Left.Negative;
    }
    else
    {
        INT32 Compare = ScriptEngineUint128Compare(&Left.Significand, &Right.Significand);
        if (Compare == 0)
        {
            SCRIPT_ENGINE_UINT128 Zero;
            Zero.High = 0;
            Zero.Low  = 0;
            return ScriptEngineSoftFloatPack(ResultKind, FALSE, 0, &Zero, RawResult);
        }
        if (Compare > 0)
        {
            ScriptEngineUint128Subtract(&Left.Significand, &Right.Significand, &Result);
            ResultSign = Left.Negative;
        }
        else
        {
            ScriptEngineUint128Subtract(&Right.Significand, &Left.Significand, &Result);
            ResultSign = Right.Negative;
        }
    }
    return ScriptEngineSoftFloatPack(ResultKind, ResultSign, ResultExponent, &Result, RawResult);
}

static BOOLEAN
ScriptEngineSoftFloatMultiply(const SCRIPT_ENGINE_SOFT_FLOAT * Left,
                              const SCRIPT_ENGINE_SOFT_FLOAT * Right,
                              UINT64 ResultKind,
                              PUINT64 RawResult)
{
    SCRIPT_ENGINE_UINT128 Result;
    Result.High = 0;
    Result.Low  = 0;
    BOOLEAN Negative = Left->Negative != Right->Negative;
    if (Left->IsZero || Right->IsZero)
    {
        return ScriptEngineSoftFloatPack(ResultKind, Negative, 0, &Result, RawResult);
    }
    ScriptEngineUint128Multiply64(Left->Significand.High, Right->Significand.High, &Result);
    INT32 Exponent = Left->Exponent + Right->Exponent;
    if (Result.High & 0x8000000000000000ULL)
    {
        Exponent++;
    }
    else
    {
        ScriptEngineUint128ShiftLeftOne(&Result);
    }
    return ScriptEngineSoftFloatPack(ResultKind, Negative, Exponent, &Result, RawResult);
}

static BOOLEAN
ScriptEngineSoftFloatDivide(const SCRIPT_ENGINE_SOFT_FLOAT * Left,
                            const SCRIPT_ENGINE_SOFT_FLOAT * Right,
                            UINT64 ResultKind,
                            PUINT64 RawResult)
{
    BOOLEAN Negative = Left->Negative != Right->Negative;
    if (Right->IsZero) return FALSE;
    if (Left->IsZero)
    {
        SCRIPT_ENGINE_UINT128 Zero;
        Zero.High = 0;
        Zero.Low  = 0;
        return ScriptEngineSoftFloatPack(ResultKind, Negative, 0, &Zero, RawResult);
    }

    UINT64 Numerator   = Left->Significand.High;
    UINT64 Denominator = Right->Significand.High;
    INT32  Exponent    = Left->Exponent - Right->Exponent;
    SCRIPT_ENGINE_UINT128 Remainder = {0, Numerator};
    SCRIPT_ENGINE_UINT128 Divisor   = {0, Denominator};
    if (Numerator < Denominator)
    {
        ScriptEngineUint128ShiftLeftOne(&Remainder);
        Exponent--;
    }

    UINT64 Quotient = 0;
    for (INT32 Bit = 63; Bit >= 0; Bit--)
    {
        if (ScriptEngineUint128Compare(&Remainder, &Divisor) >= 0)
        {
            SCRIPT_ENGINE_UINT128 Difference;
            ScriptEngineUint128Subtract(&Remainder, &Divisor, &Difference);
            Remainder.High = Difference.High;
            Remainder.Low  = Difference.Low;
            Quotient |= 1ULL << Bit;
        }
        ScriptEngineUint128ShiftLeftOne(&Remainder);
    }
    if (!ScriptEngineUint128IsZero(&Remainder)) Quotient |= 1;

    SCRIPT_ENGINE_UINT128 Result = {Quotient, 0};
    return ScriptEngineSoftFloatPack(ResultKind, Negative, Exponent, &Result, RawResult);
}

static INT32
ScriptEngineSoftFloatCompare(const SCRIPT_ENGINE_SOFT_FLOAT * Left, const SCRIPT_ENGINE_SOFT_FLOAT * Right)
{
    if (Left->IsZero && Right->IsZero) return 0;
    if (Left->Negative != Right->Negative) return Left->Negative ? -1 : 1;

    INT32 Magnitude;
    if (Left->Exponent != Right->Exponent)
        Magnitude = Left->Exponent > Right->Exponent ? 1 : -1;
    else
        Magnitude = ScriptEngineUint128Compare(&Left->Significand, &Right->Significand);
    return Left->Negative ? -Magnitude : Magnitude;
}

static BOOLEAN
ScriptEngineExecuteFloatingBinary(UINT64 Opcode,
                                  UINT64 LeftKind,
                                  UINT64 LeftBits,
                                  UINT64 RightKind,
                                  UINT64 RightBits,
                                  UINT64 ResultKind,
                                  PUINT64 Result)
{
    SCRIPT_ENGINE_SOFT_FLOAT Left;
    SCRIPT_ENGINE_SOFT_FLOAT Right;
    if (!ScriptEngineSoftFloatUnpack(LeftKind, LeftBits, &Left) ||
        !ScriptEngineSoftFloatUnpack(RightKind, RightBits, &Right)) return FALSE;

    if (Opcode == FUNC_ADD_FLOAT) return ScriptEngineSoftFloatAdd(&Left, &Right, ResultKind, Result);
    if (Opcode == FUNC_SUB_FLOAT)
    {
        Right.Negative = !Right.Negative;
        return ScriptEngineSoftFloatAdd(&Left, &Right, ResultKind, Result);
    }
    if (Opcode == FUNC_MUL_FLOAT) return ScriptEngineSoftFloatMultiply(&Left, &Right, ResultKind, Result);
    if (Opcode == FUNC_DIV_FLOAT) return ScriptEngineSoftFloatDivide(&Left, &Right, ResultKind, Result);

    INT32 Comparison = ScriptEngineSoftFloatCompare(&Left, &Right);
    if (Opcode == FUNC_GT_FLOAT) *Result = Comparison > 0;
    else if (Opcode == FUNC_LT_FLOAT) *Result = Comparison < 0;
    else if (Opcode == FUNC_EGT_FLOAT) *Result = Comparison >= 0;
    else if (Opcode == FUNC_ELT_FLOAT) *Result = Comparison <= 0;
    else if (Opcode == FUNC_EQUAL_FLOAT) *Result = Comparison == 0;
    else if (Opcode == FUNC_NEQ_FLOAT) *Result = Comparison != 0;
    else return FALSE;
    return TRUE;
}

/**
 * @brief Get the Pseudo reg value
 *
 * @param Symbol
 * @param ActionBuffer
 * @return UINT64
 */
UINT64
GetPseudoRegValue(PSYMBOL Symbol, PACTION_BUFFER ActionBuffer)
{
    switch (Symbol->Value)
    {
    case PSEUDO_REGISTER_TID:
        return ScriptEnginePseudoRegGetTid();
    case PSEUDO_REGISTER_PID:
        return ScriptEnginePseudoRegGetPid();
    case PSEUDO_REGISTER_PNAME:
        return (UINT64)ScriptEnginePseudoRegGetPname();
    case PSEUDO_REGISTER_CORE:
        return ScriptEnginePseudoRegGetCore();
    case PSEUDO_REGISTER_PROC:
        return ScriptEnginePseudoRegGetProc();
    case PSEUDO_REGISTER_THREAD:
        return ScriptEnginePseudoRegGetThread();
    case PSEUDO_REGISTER_PEB:
        return ScriptEnginePseudoRegGetPeb();
    case PSEUDO_REGISTER_TEB:
        return ScriptEnginePseudoRegGetTeb();
    case PSEUDO_REGISTER_IP:
        return ScriptEnginePseudoRegGetIp();
    case PSEUDO_REGISTER_BUFFER:
        if (ActionBuffer->CurrentAction != (UINT64)NULL)
        {
            return ScriptEnginePseudoRegGetBuffer(
                (UINT64 *)ActionBuffer->CurrentAction);
        }
        else
        {
            return (UINT64)NULL;
        }
    case PSEUDO_REGISTER_CONTEXT:
        return ActionBuffer->Context;
    case PSEUDO_REGISTER_EVENT_TAG:
        return ScriptEnginePseudoRegGetEventTag(ActionBuffer);
    case PSEUDO_REGISTER_EVENT_ID:
        return ScriptEnginePseudoRegGetEventId(ActionBuffer);
    case PSEUDO_REGISTER_EVENT_STAGE:
        return ScriptEnginePseudoRegGetEventStage(ActionBuffer);
    case PSEUDO_REGISTER_TIME:
        return ScriptEnginePseudoRegGetTime();
    case PSEUDO_REGISTER_DATE:
        return ScriptEnginePseudoRegGetDate();
    case INVALID:
#ifdef SCRIPT_ENGINE_USER_MODE
        ShowMessages("error in reading regesiter");
#endif // SCRIPT_ENGINE_USER_MODE
        return INVALID;
    default:
#ifdef SCRIPT_ENGINE_USER_MODE
        ShowMessages("unknown pseudo-register");
#endif // SCRIPT_ENGINE_USER_MODE
        return INVALID;
    }
}

/**
 * @brief Get the Value (reg, peseudo-reg, etc.)
 *
 * @param GuestRegs
 * @param ActionBuffer
 * @param ScriptGeneralRegisters
 * @param Symbol
 * @param ReturnReference
 * @return UINT64
 */
UINT64
GetValue(PGUEST_REGS                      GuestRegs,
         PACTION_BUFFER                   ActionBuffer,
         PSCRIPT_ENGINE_GENERAL_REGISTERS ScriptGeneralRegisters,
         PSYMBOL                          Symbol,
         BOOLEAN                          ReturnReference)
{
    switch (Symbol->Type)
    {
    case SYMBOL_GLOBAL_ID_TYPE:

        if (ReturnReference)
            return ((UINT64)(&ScriptGeneralRegisters->GlobalVariablesList[Symbol->Value]));
        else
            return ScriptGeneralRegisters->GlobalVariablesList[Symbol->Value];

    case SYMBOL_NUM_TYPE:

        if (ReturnReference)
            return ((UINT64)&Symbol->Value);
        else
            return Symbol->Value;

    case SYMBOL_REGISTER_TYPE:

        if (ReturnReference)
            return (UINT64)NULL; // Not reasonable, you should not dereference a register!
        else
            return GetRegValue(GuestRegs, (REGS_ENUM)Symbol->Value);

    case SYMBOL_PSEUDO_REG_TYPE:

        if (ReturnReference)
            return (UINT64)NULL; // Not reasonable, you should not dereference a pseudo-register!
        else
            return GetPseudoRegValue(Symbol, ActionBuffer);

    case SYMBOL_STACK_INDEX_TYPE:
        if (ReturnReference)
            return (UINT64)&ScriptGeneralRegisters->StackIndx;
        else
            return ScriptGeneralRegisters->StackIndx;

    case SYMBOL_STACK_BASE_INDEX_TYPE:
        if (ReturnReference)
            return (UINT64)&ScriptGeneralRegisters->StackBaseIndx;
        else
            return ScriptGeneralRegisters->StackBaseIndx;

    case SYMBOL_RETURN_VALUE_TYPE:
        if (ReturnReference)
            return (UINT64)&ScriptGeneralRegisters->ReturnValue;
        else
            return ScriptGeneralRegisters->ReturnValue;

    case SYMBOL_TEMP_TYPE:

        if (ReturnReference)
            return (UINT64)&ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx + Symbol->Value];
        else
            return ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx + Symbol->Value];

    case SYMBOL_REFERENCE_TEMP_TYPE:

        return (UINT64)&ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx + Symbol->Value];

    case SYMBOL_DEREFERENCE_TEMP_TYPE:

        return *(UINT64 *)ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx + Symbol->Value];

    case SYMBOL_FUNCTION_PARAMETER_ID_TYPE:

        if (ReturnReference)
            return (UINT64)&ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx - 3 - Symbol->Value];
        else
            return ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx - 3 - Symbol->Value];
    }

    //
    // Shouldn't reach here
    //
    return NULL64_ZERO;
}

/**
 * @brief Set the value
 *
 * @param GuestRegs
 * @param ScriptGeneralRegisters
 * @param Symbol
 * @param Value
 * @return VOID
 */
VOID
SetValue(PGUEST_REGS                       GuestRegs,
         SCRIPT_ENGINE_GENERAL_REGISTERS * ScriptGeneralRegisters,
         PSYMBOL                           Symbol,
         UINT64                            Value)
{
    switch (Symbol->Type)
    {
    case SYMBOL_GLOBAL_ID_TYPE:
        ScriptGeneralRegisters->GlobalVariablesList[Symbol->Value] = Value;
        return;
    case SYMBOL_REGISTER_TYPE:
        SetRegValueUsingSymbol(GuestRegs, Symbol, Value);
        return;

    case SYMBOL_STACK_INDEX_TYPE:
        ScriptGeneralRegisters->StackIndx = Value;
        return;

    case SYMBOL_STACK_BASE_INDEX_TYPE:
        ScriptGeneralRegisters->StackBaseIndx = Value;
        return;

    case SYMBOL_RETURN_VALUE_TYPE:
        ScriptGeneralRegisters->ReturnValue = Value;
        return;

    case SYMBOL_TEMP_TYPE:
        ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx + Symbol->Value] = Value;
        return;

    case SYMBOL_DEREFERENCE_TEMP_TYPE:
        *(UINT64 *)ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx + Symbol->Value] = Value;
        return;

    case SYMBOL_FUNCTION_PARAMETER_ID_TYPE:
        ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackBaseIndx - 3 - Symbol->Value] = Value;
        return;
    }
}

/**
 * @brief Get the operator name
 *
 * @param OperatorSymbol
 * @param BufferForName
 * @return VOID
 */
VOID
ScriptEngineGetOperatorName(PSYMBOL OperatorSymbol, CHAR * BufferForName)
{
    switch (OperatorSymbol->Value)
    {
    case FUNC_POI:
        memcpy(BufferForName, "poi", 3);
        break;
    case FUNC_DB:
        memcpy(BufferForName, "db", 2);
        break;
    case FUNC_DD:
        memcpy(BufferForName, "dd", 2);
        break;
    case FUNC_DW:
        memcpy(BufferForName, "dw", 2);
        break;
    case FUNC_DQ:
        memcpy(BufferForName, "dq", 2);
        break;
    case FUNC_HI:
        memcpy(BufferForName, "hi", 2);
        break;
    case FUNC_LOW:
        memcpy(BufferForName, "low", 3);
        break;
    case FUNC_POI_PA:
        memcpy(BufferForName, "poi_pa", 6);
        break;
    case FUNC_DB_PA:
        memcpy(BufferForName, "db_pa", 5);
        break;
    case FUNC_DD_PA:
        memcpy(BufferForName, "dd_pa", 5);
        break;
    case FUNC_DW_PA:
        memcpy(BufferForName, "dw_pa", 5);
        break;
    case FUNC_DQ_PA:
        memcpy(BufferForName, "dq_pa", 5);
        break;
    case FUNC_HI_PA:
        memcpy(BufferForName, "hi_pa", 5);
        break;
    case FUNC_LOW_PA:
        memcpy(BufferForName, "low_pa", 6);
        break;
    default:
        memcpy(BufferForName, "error", 5);
        break;
    }
}

/**
 * @brief Execute the script buffer
 *
 * @param GuestRegs General purpose registers
 * @param ActionDetail Detail of the specific action
 * @param ScriptGeneralRegisters of core specific (and global) variable holders
 * @param CodeBuffer The script buffer to be executed
 * @param Indx Script Buffer index
 * @param ErrorOperator Error in operator
 * @return BOOL
 */
BOOL
ScriptEngineExecute(PGUEST_REGS                      GuestRegs,
                    ACTION_BUFFER *                  ActionDetail,
                    PSCRIPT_ENGINE_GENERAL_REGISTERS ScriptGeneralRegisters,
                    SYMBOL_BUFFER *                  CodeBuffer,
                    UINT64 *                         Indx,
                    SYMBOL *                         ErrorOperator)
{
    PSYMBOL Operator;
    PSYMBOL Src0;
    PSYMBOL Src1;
    PSYMBOL Src2;
    PSYMBOL Src3;
    PSYMBOL Src4;

    PSYMBOL Des;
    UINT64  SrcVal0;
    UINT64  SrcVal1;
    UINT64  SrcVal2;

    UINT64 DesVal;
    BOOL   HasError = FALSE;

    Operator = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

    *ErrorOperator = *Operator;

    *Indx = *Indx + 1;

    if (Operator->Type != SYMBOL_SEMANTIC_RULE_TYPE)
    {
#ifdef SCRIPT_ENGINE_USER_MODE
        ShowMessages("err, expecting operator type\n");
        return HasError;
#endif // SCRIPT_ENGINE_USER_MODE
    };

    switch (Operator->Value)
    {
    case FUNC_TYPED_LOAD:
    {
        if (!ScriptEngineHasOperands(CodeBuffer, *Indx, 4))
        {
            HasError = TRUE;
            break;
        }
        Src0 = CodeBuffer->Head + (*Indx)++;
        Src1 = CodeBuffer->Head + (*Indx)++;
        Src2 = CodeBuffer->Head + (*Indx)++;
        Des  = CodeBuffer->Head + (*Indx)++;
        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        SrcVal1 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        SrcVal2 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        DesVal = 0;
        if ((SrcVal2 != 1 && SrcVal2 != 2 && SrcVal2 != 4 && SrcVal2 != 8) ||
            !ScriptEngineTransferMemory(ScriptGeneralRegisters, SrcVal0, SrcVal1, &DesVal, (UINT32)SrcVal2, FALSE))
            HasError = TRUE;
        else
            SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);
        break;
    }

    case FUNC_TYPED_STORE:
    {
        if (!ScriptEngineHasOperands(CodeBuffer, *Indx, 4))
        {
            HasError = TRUE;
            break;
        }
        Src0 = CodeBuffer->Head + (*Indx)++;
        Src1 = CodeBuffer->Head + (*Indx)++;
        Src2 = CodeBuffer->Head + (*Indx)++;
        Src3 = CodeBuffer->Head + (*Indx)++;
        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        SrcVal1 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        SrcVal2 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        DesVal  = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src3, FALSE);
        if ((DesVal != 1 && DesVal != 2 && DesVal != 4 && DesVal != 8) ||
            !ScriptEngineTransferMemory(ScriptGeneralRegisters, SrcVal1, SrcVal2, &SrcVal0, (UINT32)DesVal, TRUE))
            HasError = TRUE;
        break;
    }

    case FUNC_AGGREGATE_ZERO:
    {
        BYTE ZeroBuffer[64] = {0};
        UINT64 Done = 0;
        if (!ScriptEngineHasOperands(CodeBuffer, *Indx, 3))
        {
            HasError = TRUE;
            break;
        }
        Src0 = CodeBuffer->Head + (*Indx)++;
        Src1 = CodeBuffer->Head + (*Indx)++;
        Src2 = CodeBuffer->Head + (*Indx)++;
        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        SrcVal1 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        SrcVal2 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        while (Done < SrcVal2 && !HasError)
        {
            UINT32 Chunk = (UINT32)((SrcVal2 - Done) > sizeof(ZeroBuffer) ? sizeof(ZeroBuffer) : (SrcVal2 - Done));
            if (!ScriptEngineTransferMemory(ScriptGeneralRegisters, SrcVal0 + Done, SrcVal1, ZeroBuffer, Chunk, TRUE))
                HasError = TRUE;
            Done += Chunk;
        }
        break;
    }

    case FUNC_AGGREGATE_COPY:
    {
        BYTE MovingBuffer[64];
        UINT64 Done = 0;
        BOOLEAN Backward;
        if (!ScriptEngineHasOperands(CodeBuffer, *Indx, 5))
        {
            HasError = TRUE;
            break;
        }
        Src0 = CodeBuffer->Head + (*Indx)++;
        Src1 = CodeBuffer->Head + (*Indx)++;
        Src2 = CodeBuffer->Head + (*Indx)++;
        Src3 = CodeBuffer->Head + (*Indx)++;
        Src4 = CodeBuffer->Head + (*Indx)++;
        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        SrcVal1 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        SrcVal2 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        DesVal  = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src3, FALSE);
        {
            UINT64 Size = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src4, FALSE);
            Backward = SrcVal1 == DesVal && SrcVal0 > SrcVal2 && SrcVal0 < SrcVal2 + Size;
            while (Done < Size && !HasError)
            {
                UINT32 Chunk = (UINT32)((Size - Done) > sizeof(MovingBuffer) ? sizeof(MovingBuffer) : (Size - Done));
                UINT64 Offset = Backward ? Size - Done - Chunk : Done;
                if (!ScriptEngineTransferMemory(ScriptGeneralRegisters, SrcVal2 + Offset, DesVal, MovingBuffer, Chunk, FALSE) ||
                    !ScriptEngineTransferMemory(ScriptGeneralRegisters, SrcVal0 + Offset, SrcVal1, MovingBuffer, Chunk, TRUE))
                    HasError = TRUE;
                Done += Chunk;
            }
        }
        break;
    }

    case FUNC_ED:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEd(SrcVal1, (DWORD)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EB:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEb(SrcVal1, (BYTE)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EQ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEq(SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_ED_PA:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEdPa(SrcVal1, (DWORD)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EB_PA:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEbPa(SrcVal1, (BYTE)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EQ_PA:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEqPa(SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_INTERLOCKED_EXCHANGE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedExchange((volatile long long *)SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_INTERLOCKED_EXCHANGE_ADD:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedExchangeAdd((volatile long long *)SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_INTERLOCKED_COMPARE_EXCHANGE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedCompareExchange((volatile long long *)SrcVal2, SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EVENT_INJECT_ERROR_CODE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        ScriptEngineFunctionEventInjectErrorCode((UINT32)SrcVal2, (UINT32)SrcVal1, (UINT32)SrcVal0, &HasError);

        break;

    case FUNC_MEMCPY:

        if (!ScriptEngineHasOperands(CodeBuffer, *Indx, 3))
        {
            HasError = TRUE;
            break;
        }

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);

        if (SrcVal0 == 0 || SrcVal0 > 0xffffffffULL)
        {
            HasError = TRUE;
            break;
        }

        if (ScriptEngineTypedLocalRangeIsValid(ScriptGeneralRegisters, SrcVal2, (UINT32)SrcVal0) &&
            ScriptEngineTypedLocalRangeIsValid(ScriptGeneralRegisters, SrcVal1, (UINT32)SrcVal0))
        {
            memmove((PVOID)SrcVal2, (PVOID)SrcVal1, (SIZE_T)SrcVal0);
        }
        else
        {
            ScriptEngineFunctionMemcpy(SrcVal2, SrcVal1, (UINT32)SrcVal0, &HasError);
        }

        break;

    case FUNC_MEMCPY_PA:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);

        ScriptEngineFunctionMemcpyPa(SrcVal2, SrcVal1, (UINT32)SrcVal0, &HasError);

        break;

    case FUNC_SPINLOCK_LOCK_CUSTOM_WAIT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        ScriptEngineFunctionSpinlockLockCustomWait((volatile long *)SrcVal1, (UINT32)SrcVal0, &HasError);

        break;

    case FUNC_EVENT_INJECT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        ScriptEngineFunctionEventInject((UINT32)SrcVal1, (UINT32)SrcVal0, &HasError);

        break;

    case FUNC_PAUSE:

        ScriptEngineFunctionPause(ActionDetail,
                                  GuestRegs);

        break;

    case FUNC_LBR_CHECK:

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionLbrCheck();

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_LBR_SAVE:

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionLbrSave();

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_LBR_PRINT:
    case FUNC_LBR_DUMP:

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionLbrPrint();

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_LBR_RESTORE:

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionLbrRestore();

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_LBR_RESTORE_BY_FILTER:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionLbrRestoreByFilter((unsigned long long)SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_FLUSH:

        ScriptEngineFunctionFlush();

        break;

    case FUNC_EVENT_TRACE_INSTRUMENTATION_STEP:
    case FUNC_EVENT_TRACE_INSTRUMENTATION_STEP_IN:

        ScriptEngineFunctionEventTraceInstrumentationStep();

        break;

    case FUNC_EVENT_TRACE_STEP:
    case FUNC_EVENT_TRACE_STEP_IN:

        ScriptEngineFunctionEventTraceStepIn();

        break;

    case FUNC_EVENT_TRACE_STEP_OUT:

        //
        // To be implemented!
        //

        break;

    case FUNC_EVENT_SC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        ScriptEngineFunctionShortCircuitingEvent(SrcVal0, ActionDetail);

        break;

    case FUNC_OR:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 | SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_INC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        DesVal = SrcVal0 + 1;

        Des = Src0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DEC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        DesVal = SrcVal0 - 1;

        Des = Src0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_XOR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 ^ SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_AND:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 & SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_ASR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 >> SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_ASL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 << SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_ADD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 + SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_SUB:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 - SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_MUL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 * SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DIV:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (SrcVal0 == 0)
        {
            HasError = TRUE;
            break;
        }

        DesVal = SrcVal1 / SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_MOD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (SrcVal0 == 0)
        {
            HasError = TRUE;
            break;
        }

        DesVal = SrcVal1 % SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_GT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 > (INT64)SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_LT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 < (INT64)SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EGT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 >= (INT64)SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_ELT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = (INT64)SrcVal1 <= (INT64)SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_EQUAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 == SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_NEQ:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 != SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_POI:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordPoi((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                        &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DB:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDb((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDd((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                       &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DW:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDw((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DQ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDq((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_POI_PA:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordPoiPa((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                          &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DB_PA:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDbPa((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                         &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DD_PA:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDdPa((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                         &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DW_PA:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDwPa((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                         &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DQ_PA:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDqPa((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                         &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_NOT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ~SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_REFERENCE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        //
        // It's reference, we need an address
        //
        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, TRUE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_PHYSICAL_TO_VIRTUAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionPhysicalToVirtual(GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE));

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_VIRTUAL_TO_PHYSICAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionVirtualToPhysical(GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE));

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_CHECK_ADDRESS:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (ScriptEngineFunctionCheckAddress(SrcVal0, sizeof(BYTE)))
            DesVal = 1; // TRUE
        else
            DesVal = 0; // FALSE

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_STRLEN:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        }

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionStrlen((const char *)SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DISASSEMBLE_LEN:
    case FUNC_DISASSEMBLE_LEN64:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionDisassembleLen((PVOID)SrcVal0, FALSE);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_DISASSEMBLE_LEN32:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionDisassembleLen((PVOID)SrcVal0, TRUE);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_WCSLEN:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        }

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionWcslen((const wchar_t *)SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_MICROSLEEP:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        ScriptEngineFunctionMicroSleep(SrcVal0);
        break;

    case FUNC_RDTSC:

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionRdtsc();

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_RDTSCP:

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionRdtscp();

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);
        break;

    case FUNC_INTERLOCKED_INCREMENT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedIncrement((volatile long long *)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_INTERLOCKED_DECREMENT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedDecrement((volatile long long *)SrcVal0, &HasError);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_NEG:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = -(INT64)SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_HI:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordHi((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_LOW:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordLow((PUINT64)GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE),
                                        &HasError);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_MOV_FLOAT:
        if (*Indx > CodeBuffer->Pointer || CodeBuffer->Pointer - *Indx < 2)
        {
            HasError = TRUE;
            break;
        }

        Src0 = CodeBuffer->Head + (*Indx)++;
        Des  = CodeBuffer->Head + (*Indx)++;

        if (!ScriptEngineFloatingSymbolIsReadable(ScriptGeneralRegisters, Src0) ||
            !ScriptEngineFloatingSymbolIsWritable(ScriptGeneralRegisters, Des) ||
            Src0->Len != Des->Len)
        {
            HasError = TRUE;
            break;
        }

        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        if (Src0->Len == SYMBOL_VALUE_KIND_FLOAT32)
        {
            SrcVal0 &= 0xffffffffULL;
        }
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, SrcVal0);
        break;

    case FUNC_CONVERT_FLOAT:
    {
        if (*Indx > CodeBuffer->Pointer || CodeBuffer->Pointer - *Indx < 2)
        {
            HasError = TRUE;
            break;
        }

        Src0 = CodeBuffer->Head + (*Indx)++;
        Des  = CodeBuffer->Head + (*Indx)++;
        if (!ScriptEngineFloatingSymbolIsReadable(ScriptGeneralRegisters, Src0) ||
            !ScriptEngineFloatingSymbolIsWritable(ScriptGeneralRegisters, Des) ||
            Src0->Len == Des->Len)
        {
            HasError = TRUE;
            break;
        }

        SCRIPT_ENGINE_SOFT_FLOAT ConvertedValue;
        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        if (!ScriptEngineSoftFloatUnpack(Src0->Len, SrcVal0, &ConvertedValue) ||
            !ScriptEngineSoftFloatPack(Des->Len,
                                       ConvertedValue.Negative,
                                       ConvertedValue.Exponent,
                                       &ConvertedValue.Significand,
                                       &DesVal))
        {
            HasError = TRUE;
            break;
        }
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);
        break;
    }

    case FUNC_CAST_SCALAR:
    {
        UINT64 SourceType;
        UINT64 DestinationType;
        if (!ScriptEngineHasOperands(CodeBuffer, *Indx, 4))
        {
            HasError = TRUE;
            break;
        }
        Src0 = CodeBuffer->Head + (*Indx)++;
        Des  = CodeBuffer->Head + (*Indx)++;
        Src1 = CodeBuffer->Head + (*Indx)++;
        Src2 = CodeBuffer->Head + (*Indx)++;
        SourceType      = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        DestinationType = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);

        if ((ScriptEngineScalarTypeIsFloating(SourceType) ?
                 !ScriptEngineFloatingSymbolIsReadable(ScriptGeneralRegisters, Src0) :
                 Src0->Len != SYMBOL_VALUE_KIND_INTEGER) ||
            (ScriptEngineScalarTypeIsFloating(DestinationType) ?
                 !ScriptEngineFloatingSymbolIsWritable(ScriptGeneralRegisters, Des) :
                 !ScriptEngineIntegerSymbolIsWritable(ScriptGeneralRegisters, Des)))
        {
            HasError = TRUE;
            break;
        }

        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        if (!ScriptEngineCastScalar(SrcVal0, SourceType, DestinationType, &DesVal))
        {
            HasError = TRUE;
            break;
        }
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);
        break;
    }

    case FUNC_ADD_TYPED:
    case FUNC_SUB_TYPED:
    case FUNC_MUL_TYPED:
    case FUNC_DIV_TYPED:
    case FUNC_MOD_TYPED:
    case FUNC_BITWISE_AND_TYPED:
    case FUNC_BITWISE_OR_TYPED:
    case FUNC_BITWISE_XOR_TYPED:
    case FUNC_SHIFT_LEFT_TYPED:
    case FUNC_SHIFT_RIGHT_TYPED:
    case FUNC_GT_TYPED:
    case FUNC_LT_TYPED:
    case FUNC_EGT_TYPED:
    case FUNC_ELT_TYPED:
    case FUNC_EQUAL_TYPED:
    case FUNC_NEQ_TYPED:
    {
        UINT64 OperationType;
        if (!ScriptEngineHasOperands(CodeBuffer, *Indx, 4))
        {
            HasError = TRUE;
            break;
        }
        Src0 = CodeBuffer->Head + (*Indx)++;
        Src1 = CodeBuffer->Head + (*Indx)++;
        Des  = CodeBuffer->Head + (*Indx)++;
        Src2 = CodeBuffer->Head + (*Indx)++;
        OperationType = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        if (Src0->Len != SYMBOL_VALUE_KIND_INTEGER || Src1->Len != SYMBOL_VALUE_KIND_INTEGER ||
            !ScriptEngineIntegerSymbolIsWritable(ScriptGeneralRegisters, Des))
        {
            HasError = TRUE;
            break;
        }
        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        SrcVal1 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        if (!ScriptEngineExecuteTypedBinary(Operator->Value, SrcVal0, SrcVal1, OperationType, &DesVal))
        {
            HasError = TRUE;
            break;
        }
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);
        break;
    }

    case FUNC_NEG_TYPED:
    case FUNC_BITWISE_NOT_TYPED:
    case FUNC_LOGICAL_NOT_TYPED:
    {
        UINT64 OperationType;
        if (!ScriptEngineHasOperands(CodeBuffer, *Indx, 3))
        {
            HasError = TRUE;
            break;
        }
        Src0 = CodeBuffer->Head + (*Indx)++;
        Des  = CodeBuffer->Head + (*Indx)++;
        Src1 = CodeBuffer->Head + (*Indx)++;
        OperationType = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        if ((!ScriptEngineScalarTypeIsInteger(OperationType) && OperationType != SCRIPT_SCALAR_TYPE_POINTER &&
             !(Operator->Value == FUNC_LOGICAL_NOT_TYPED && ScriptEngineScalarTypeIsFloating(OperationType))) ||
            (ScriptEngineScalarTypeIsFloating(OperationType) ?
                 !ScriptEngineFloatingSymbolIsReadable(ScriptGeneralRegisters, Src0) :
                 Src0->Len != SYMBOL_VALUE_KIND_INTEGER) ||
            !ScriptEngineIntegerSymbolIsWritable(ScriptGeneralRegisters, Des))
        {
            HasError = TRUE;
            break;
        }
        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        if (Operator->Value == FUNC_LOGICAL_NOT_TYPED)
        {
            if (OperationType == SCRIPT_SCALAR_TYPE_F32)
                DesVal = (SrcVal0 & 0x7fffffffULL) == 0;
            else if (OperationType == SCRIPT_SCALAR_TYPE_F64)
                DesVal = (SrcVal0 & 0x7fffffffffffffffULL) == 0;
            else
                DesVal = SrcVal0 == 0;
        }
        else if (Operator->Value == FUNC_NEG_TYPED)
            DesVal = ScriptEngineNormalizeInteger(0ULL - SrcVal0, OperationType);
        else
            DesVal = ScriptEngineNormalizeInteger(~SrcVal0, OperationType);
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);
        break;
    }

    case FUNC_POINTER_DIFF:
    {
        UINT64 ElementSize;
        UINT64 Magnitude;
        BOOLEAN Negative;
        if (!ScriptEngineHasOperands(CodeBuffer, *Indx, 4))
        {
            HasError = TRUE;
            break;
        }
        Src0 = CodeBuffer->Head + (*Indx)++;
        Src1 = CodeBuffer->Head + (*Indx)++;
        Des  = CodeBuffer->Head + (*Indx)++;
        Src2 = CodeBuffer->Head + (*Indx)++;
        ElementSize = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        if (!ElementSize || Src0->Len != SYMBOL_VALUE_KIND_INTEGER ||
            Src1->Len != SYMBOL_VALUE_KIND_INTEGER ||
            !ScriptEngineIntegerSymbolIsWritable(ScriptGeneralRegisters, Des))
        {
            HasError = TRUE;
            break;
        }
        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        SrcVal1 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        Negative = SrcVal1 < SrcVal0;
        Magnitude = Negative ? SrcVal0 - SrcVal1 : SrcVal1 - SrcVal0;
        if (Magnitude % ElementSize)
        {
            HasError = TRUE;
            break;
        }
        Magnitude /= ElementSize;
        if ((!Negative && Magnitude > 0x7fffffffffffffffULL) ||
            (Negative && Magnitude > 0x8000000000000000ULL))
        {
            HasError = TRUE;
            break;
        }
        DesVal = Negative ? 0ULL - Magnitude : Magnitude;
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);
        break;
    }

    case FUNC_NEG_FLOAT:
        if (*Indx > CodeBuffer->Pointer || CodeBuffer->Pointer - *Indx < 2)
        {
            HasError = TRUE;
            break;
        }

        Src0 = CodeBuffer->Head + (*Indx)++;
        Des  = CodeBuffer->Head + (*Indx)++;

        if (!ScriptEngineFloatingSymbolIsReadable(ScriptGeneralRegisters, Src0) ||
            !ScriptEngineFloatingSymbolIsWritable(ScriptGeneralRegisters, Des) ||
            Src0->Len != Des->Len)
        {
            HasError = TRUE;
            break;
        }

        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        if (Src0->Len == SYMBOL_VALUE_KIND_FLOAT32)
        {
            DesVal = (SrcVal0 ^ 0x80000000ULL) & 0xffffffffULL;
        }
        else
        {
            DesVal = SrcVal0 ^ 0x8000000000000000ULL;
        }
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);
        break;

    case FUNC_ADD_FLOAT:
    case FUNC_SUB_FLOAT:
    case FUNC_MUL_FLOAT:
    case FUNC_DIV_FLOAT:
    case FUNC_GT_FLOAT:
    case FUNC_LT_FLOAT:
    case FUNC_EGT_FLOAT:
    case FUNC_ELT_FLOAT:
    case FUNC_EQUAL_FLOAT:
    case FUNC_NEQ_FLOAT:
    {
        if (*Indx > CodeBuffer->Pointer || CodeBuffer->Pointer - *Indx < 3)
        {
            HasError = TRUE;
            break;
        }

        Src0 = CodeBuffer->Head + (*Indx)++;
        Src1 = CodeBuffer->Head + (*Indx)++;
        Des  = CodeBuffer->Head + (*Indx)++;

        BOOLEAN IsComparison = ScriptEngineIsFloatingComparisonOpcode(Operator->Value);
        UINT64  ExpectedResultKind =
            Src0->Len == SYMBOL_VALUE_KIND_FLOAT64 || Src1->Len == SYMBOL_VALUE_KIND_FLOAT64 ?
                SYMBOL_VALUE_KIND_FLOAT64 : SYMBOL_VALUE_KIND_FLOAT32;

        if (!ScriptEngineFloatingSymbolIsReadable(ScriptGeneralRegisters, Src0) ||
            !ScriptEngineFloatingSymbolIsReadable(ScriptGeneralRegisters, Src1) ||
            (IsComparison ?
                 !ScriptEngineIntegerSymbolIsWritable(ScriptGeneralRegisters, Des) :
                 (!ScriptEngineFloatingSymbolIsWritable(ScriptGeneralRegisters, Des) ||
                  Des->Len != ExpectedResultKind)))
        {
            HasError = TRUE;
            break;
        }

        SrcVal0 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        SrcVal1 = GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        if (!ScriptEngineExecuteFloatingBinary(Operator->Value,
                                               Src1->Len,
                                               SrcVal1,
                                               Src0->Len,
                                               SrcVal0,
                                               IsComparison ? SYMBOL_VALUE_KIND_INTEGER : Des->Len,
                                               &DesVal))
        {
            HasError = TRUE;
            break;
        }

        if (!IsComparison && Des->Len == SYMBOL_VALUE_KIND_FLOAT32)
        {
            DesVal &= 0xffffffffULL;
        }
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);
        break;
    }

    case FUNC_MOV:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal0;

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_PRINT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionPrint(ActionDetail->Tag,
                                  ActionDetail->ImmediatelySendTheResults,
                                  SrcVal0);
        break;

    case FUNC_TEST_STATEMENT:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionTestStatement(ActionDetail->Tag,
                                          ActionDetail->ImmediatelySendTheResults,
                                          SrcVal0);
        break;

    case FUNC_SPINLOCK_LOCK:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionSpinlockLock((volatile LONG *)SrcVal0, &HasError);

        break;

    case FUNC_SPINLOCK_UNLOCK:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionSpinlockUnlock((volatile LONG *)SrcVal0, &HasError);

        break;

    case FUNC_EVENT_ENABLE:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        ScriptEngineFunctionEventEnable(SrcVal0);

        break;

    case FUNC_EVENT_DISABLE:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        ScriptEngineFunctionEventDisable(SrcVal0);

        break;

    case FUNC_EVENT_CLEAR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        ScriptEngineFunctionEventClear(SrcVal0);

        break;

    case FUNC_FORMATS:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionFormats(
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            SrcVal0);

        break;

    case FUNC_JZ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        if (SrcVal1 == 0)
            *Indx = SrcVal0;

        break;

    case FUNC_JNZ:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;
        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);

        if (SrcVal1 != 0)
            *Indx = SrcVal0;

        break;

    case FUNC_JMP:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        *Indx = SrcVal0;

        break;

    case FUNC_PUSH:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackIndx] = SrcVal0;
        ScriptGeneralRegisters->StackIndx++;

        break;

    case FUNC_POP:
        ScriptGeneralRegisters->StackIndx--;

        SrcVal0 = ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackIndx];

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SetValue(GuestRegs, ScriptGeneralRegisters, Des, SrcVal0);

        break;

    case FUNC_CALL:
        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        *Indx = *Indx + 1;

        ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackIndx] = *Indx;

        ScriptGeneralRegisters->StackIndx++;

        *Indx = SrcVal0;
        break;

    case FUNC_RET:

        ScriptGeneralRegisters->StackIndx--;

        *Indx = ScriptGeneralRegisters->StackBuffer[ScriptGeneralRegisters->StackIndx];
        break;
    case FUNC_STRCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        }

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionStrcmp((const char *)SrcVal1, (const char *)SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_WCSCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src0->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src0->Len) /
                         sizeof(SYMBOL));
            SrcVal0 = (UINT64)&Src0->Value;
        }
        else
        {
            SrcVal0 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);
        }

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionWcscmp((const wchar_t *)SrcVal1, (const wchar_t *)SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_MEMCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        }

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src2->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src2->Len) /
                         sizeof(SYMBOL));
            SrcVal2 = (UINT64)&Src2->Value;
        }
        else
        {
            SrcVal2 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionMemcmp((const char *)SrcVal2, (const char *)SrcVal1, SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_STRNCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        }

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src2->Type == SYMBOL_STRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src2->Len) /
                         sizeof(SYMBOL));
            SrcVal2 = (UINT64)&Src2->Value;
        }
        else
        {
            SrcVal2 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionStrncmp((const char *)SrcVal2, (const char *)SrcVal1, SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_WCSNCMP:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src1->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src1->Len) /
                         sizeof(SYMBOL));
            SrcVal1 = (UINT64)&Src1->Value;
        }
        else
        {
            SrcVal1 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src1, FALSE);
        }

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (Src2->Type == SYMBOL_WSTRING_TYPE)
        {
            *Indx =
                *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src2->Len) /
                         sizeof(SYMBOL));
            SrcVal2 = (UINT64)&Src2->Value;
        }
        else
        {
            SrcVal2 =
                GetValue(GuestRegs, ActionDetail, ScriptGeneralRegisters, Src2, FALSE);
        }

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionWcsncmp((const wchar_t *)SrcVal2, (const wchar_t *)SrcVal1, SrcVal0);

        SetValue(GuestRegs, ScriptGeneralRegisters, Des, DesVal);

        break;

    case FUNC_PRINTF:

        if (*Indx >= CodeBuffer->Pointer)
        {
            HasError = TRUE;
            break;
        }

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        //
        // Call the target function
        //

        UINT64 RemainingFormatSymbols = CodeBuffer->Pointer - *Indx;
        if ((Src0->Type & 0xffffffffULL) != SYMBOL_STRING_TYPE ||
            Src0->Len == 0 ||
            Src0->Len > RemainingFormatSymbols * sizeof(SYMBOL) ||
            !memchr((PCHAR)&Src0->Value, '\0', (SIZE_T)Src0->Len) ||
            ((SIZE_SYMBOL_WITHOUT_LEN + Src0->Len) / sizeof(SYMBOL)) >= CodeBuffer->Pointer - *Indx)
        {
            HasError = TRUE;
            break;
        }

        *Indx =
            *Indx + ((SIZE_SYMBOL_WITHOUT_LEN + Src0->Len) /
                     sizeof(SYMBOL));

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if ((Src1->Type & 0xffffffffULL) != SYMBOL_VARIABLE_COUNT_TYPE ||
            Src1->Value > CodeBuffer->Pointer - *Indx)
        {
            HasError = TRUE;
            break;
        }

        Src2 = NULL;

        if (Src1->Value > 0)
        {
            Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                             (unsigned long long)(*Indx * sizeof(SYMBOL)));

            *Indx = *Indx + Src1->Value;
        }

        for (UINT64 ArgumentIndex = 0; ArgumentIndex < Src1->Value; ArgumentIndex++)
        {
            PSYMBOL Argument = Src2 + ArgumentIndex;
            UINT64  BaseType = Argument->Type & 0xffffffffULL;

            if (BaseType != SYMBOL_STRING_TYPE && BaseType != SYMBOL_WSTRING_TYPE &&
                Argument->Len != SYMBOL_VALUE_KIND_INTEGER &&
                !ScriptEngineFloatingSymbolIsReadable(ScriptGeneralRegisters, Argument))
            {
                HasError = TRUE;
                break;
            }
        }

        if (HasError)
        {
            break;
        }

        ScriptEngineFunctionPrintf(
            GuestRegs,
            ActionDetail,
            ScriptGeneralRegisters,
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            (char *)&Src0->Value,
            Src1->Value,
            Src2,
            (BOOLEAN *)&HasError);

        break;
    }

    //
    // Return the result of whether error detected or not
    //
    return HasError;
}
