package hwdbg.script

import chisel3._
import chisel3.util._

/**
 * @brief
 *   The structure of HWDBG_SHORT_SYMBOL used in script engine of HyperDbg
 */
class HwdbgShortSymbol(
    scriptVariableLength: Int
) extends Bundle {
  
  //
  // Ensure that the script variable length is at least 8 bits or 1 byte
  //
  require(
    scriptVariableLength >= 8,
    f"err, the minimum script variable length is 8 bits (1 byte)." 
  )

  val Type = UInt(scriptVariableLength.W) // long long unsigned is 64 bits but it can be dynamic
  val Value = UInt(scriptVariableLength.W) // long long unsigned is 64 bits but it can be dynamic
}

/**
 * @brief
 *   Constant values for the script engine
 */
object ScriptConstants {
  val SYMBOL_MEM_VALID_CHECK_MASK = 1 << 31
  val INVALID = 0x80000000
  val LALR_ACCEPT = 0x7fffffff
  val SYMBOL_VALUE_KIND_INTEGER = 0
  val SYMBOL_VALUE_KIND_FLOAT32 = 1
  val SYMBOL_VALUE_KIND_FLOAT64 = 2
  val SCRIPT_SCALAR_TYPE_INVALID = 0
  val SCRIPT_SCALAR_TYPE_BOOL = 1
  val SCRIPT_SCALAR_TYPE_I8 = 2
  val SCRIPT_SCALAR_TYPE_I16 = 3
  val SCRIPT_SCALAR_TYPE_I32 = 4
  val SCRIPT_SCALAR_TYPE_I64 = 5
  val SCRIPT_SCALAR_TYPE_U8 = 6
  val SCRIPT_SCALAR_TYPE_U16 = 7
  val SCRIPT_SCALAR_TYPE_U32 = 8
  val SCRIPT_SCALAR_TYPE_U64 = 9
  val SCRIPT_SCALAR_TYPE_F32 = 10
  val SCRIPT_SCALAR_TYPE_F64 = 11
  val SCRIPT_SCALAR_TYPE_POINTER = 12
  val SCRIPT_SCALAR_TYPE_F80 = 13
}

/**
 * @brief
 *   Constant type values for the script engine
 */
object ScriptConstantTypes {
  object ScriptDataTypes extends ChiselEnum {
    val symbolUndefined, symbolGlobalIdType, symbolLocalIdType, symbolNumType, symbolRegisterType, symbolPseudoRegType, symbolSemanticRuleType, symbolTempType, symbolStringType, symbolVariableCountType, symbolInvalid, symbolWstringType, symbolFunctionParameterIdType, symbolReturnAddressType, symbolFunctionParameterType, symbolStackIndexType, symbolStackBaseIndexType, symbolReturnValueType  = Value
  }
}

object ScriptEvalFunc {
  object ScriptOperators extends ChiselEnum {
    val sFuncUndefined, sFuncInc, sFuncDec, sFuncReference, sFuncOr, sFuncXor, sFuncAnd, sFuncAsr, sFuncAsl, sFuncAdd, sFuncSub, sFuncMul, sFuncDiv, sFuncMod, sFuncGt, sFuncLt, sFuncEgt, sFuncElt, sFuncEqual, sFuncNeq, sFuncJmp, sFuncJz, sFuncJnz, sFuncMov, sFuncStart_of_do_while, sFuncStart_of_do_while_commands, sFuncEnd_of_do_while, sFuncStart_of_for, sFuncFor_inc_dec, sFuncStart_of_for_ommands, sFuncEnd_of_if, sFuncIgnore_lvalue, sFuncPush, sFuncPop, sFuncCall, sFuncRet, sFuncPrint, sFuncFormats, sFuncEvent_enable, sFuncEvent_disable, sFuncEvent_clear, sFuncTest_statement, sFuncSpinlock_lock, sFuncSpinlock_unlock, sFuncEvent_sc, sFuncMicrosleep, sFuncPrintf, sFuncPause, sFuncFlush, sFuncEvent_trace_step, sFuncEvent_trace_step_in, sFuncEvent_trace_step_out, sFuncEvent_trace_instrumentation_step, sFuncEvent_trace_instrumentation_step_in, sFuncRdtsc, sFuncRdtscp, sFuncLbr_save, sFuncLbr_dump, sFuncLbr_print, sFuncLbr_restore, sFuncLbr_check, sFuncSpinlock_lock_custom_wait, sFuncEvent_inject, sFuncPoi, sFuncDb, sFuncDd, sFuncDw, sFuncDq, sFuncNeg, sFuncHi, sFuncLow, sFuncNot, sFuncCheck_address, sFuncDisassemble_len, sFuncDisassemble_len32, sFuncDisassemble_len64, sFuncInterlocked_increment, sFuncInterlocked_decrement, sFuncPhysical_to_virtual, sFuncVirtual_to_physical, sFuncPoi_pa, sFuncHi_pa, sFuncLow_pa, sFuncDb_pa, sFuncDd_pa, sFuncDw_pa, sFuncDq_pa, sFuncLbr_restore_by_filter, sFuncEd, sFuncEb, sFuncEq, sFuncInterlocked_exchange, sFuncInterlocked_exchange_add, sFuncEb_pa, sFuncEd_pa, sFuncEq_pa, sFuncInterlocked_compare_exchange, sFuncStrlen, sFuncStrcmp, sFuncMemcmp, sFuncStrncmp, sFuncWcslen, sFuncWcscmp, sFuncEvent_inject_error_code, sFuncMemcpy, sFuncMemcpy_pa, sFuncWcsncmp, sFuncStruct_forward_declaration, sFuncStruct_definition_begin, sFuncStruct_definition_end, sFuncStruct_variable_declaration, sFuncStruct_member_declaration, sFuncTypedef_declaration, sFuncStruct_pointer, sFuncStruct_array_dimension, sFuncStruct_declarator_complete, sFuncTyped_load, sFuncTyped_store, sFuncAggregate_copy, sFuncAggregate_zero, sFuncStruct_initializer_begin, sFuncStruct_initializer_end, sFuncStruct_pointer_cast, sFuncMember_address, sFuncMember_read, sFuncMember_dot_lvalue, sFuncMember_arrow_lvalue, sFuncMember_dot_read, sFuncMember_arrow_read, sFuncMov_float, sFuncNeg_float, sFuncAdd_float, sFuncSub_float, sFuncMul_float, sFuncDiv_float, sFuncGt_float, sFuncLt_float, sFuncEgt_float, sFuncElt_float, sFuncEqual_float, sFuncNeq_float, sFuncConvert_float, sFuncCast_scalar, sFuncAdd_typed, sFuncSub_typed, sFuncMul_typed, sFuncDiv_typed, sFuncMod_typed, sFuncBitwise_and_typed, sFuncBitwise_or_typed, sFuncBitwise_xor_typed, sFuncShift_left_typed, sFuncShift_right_typed, sFuncGt_typed, sFuncLt_typed, sFuncEgt_typed, sFuncElt_typed, sFuncEqual_typed, sFuncNeq_typed, sFuncNeg_typed, sFuncBitwise_not_typed, sFuncLogical_not_typed, sFuncPointer_diff = Value
  }
} 