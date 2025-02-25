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
    val sFuncUndefined, sFuncInc, sFuncDec, sFuncReference, sFuncDereference, sFuncOr, sFuncXor, sFuncAnd, sFuncAsr, sFuncAsl, sFuncAdd, sFuncSub, sFuncMul, sFuncDiv, sFuncMod, sFuncGt, sFuncLt, sFuncEgt, sFuncElt, sFuncEqual, sFuncNeq, sFuncJmp, sFuncJz, sFuncJnz, sFuncMov, sFuncStart_of_do_while, sFuncStart_of_do_while_commands, sFuncEnd_of_do_while, sFuncStart_of_for, sFuncFor_inc_dec, sFuncStart_of_for_ommands, sFuncEnd_of_if, sFuncIgnore_lvalue, sFuncPush, sFuncPop, sFuncCall, sFuncRet, sFuncPrint, sFuncFormats, sFuncEvent_enable, sFuncEvent_disable, sFuncEvent_clear, sFuncTest_statement, sFuncSpinlock_lock, sFuncSpinlock_unlock, sFuncEvent_sc, sFuncPrintf, sFuncPause, sFuncFlush, sFuncEvent_trace_step, sFuncEvent_trace_step_in, sFuncEvent_trace_step_out, sFuncEvent_trace_instrumentation_step, sFuncEvent_trace_instrumentation_step_in, sFuncSpinlock_lock_custom_wait, sFuncEvent_inject, sFuncPoi, sFuncDb, sFuncDd, sFuncDw, sFuncDq, sFuncNeg, sFuncHi, sFuncLow, sFuncNot, sFuncCheck_address, sFuncDisassemble_len, sFuncDisassemble_len32, sFuncDisassemble_len64, sFuncInterlocked_increment, sFuncInterlocked_decrement, sFuncPhysical_to_virtual, sFuncVirtual_to_physical, sFuncPoi_pa, sFuncHi_pa, sFuncLow_pa, sFuncDb_pa, sFuncDd_pa, sFuncDw_pa, sFuncDq_pa, sFuncEd, sFuncEb, sFuncEq, sFuncInterlocked_exchange, sFuncInterlocked_exchange_add, sFuncEb_pa, sFuncEd_pa, sFuncEq_pa, sFuncInterlocked_compare_exchange, sFuncStrlen, sFuncStrcmp, sFuncMemcmp, sFuncStrncmp, sFuncWcslen, sFuncWcscmp, sFuncEvent_inject_error_code, sFuncMemcpy, sFuncMemcpy_pa, sFuncWcsncmp = Value
  }
} 