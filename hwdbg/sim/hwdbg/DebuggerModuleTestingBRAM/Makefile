# Makefile

TOPLEVEL_LANG = verilog

VERILOG_SOURCES += $(shell pwd)/../../../generated/DebuggerModuleTestingBRAM.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/InitRegMemFromFile.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/DebuggerMain.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/SendReceiveSynchronizer.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/DebuggerPacketReceiver.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/DebuggerPacketSender.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/DebuggerPacketInterpreter.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/InterpreterInstanceInfo.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/InterpreterSendSuccessOrError.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/ScriptExecutionEngine.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/InterpreterScriptBufferHandler.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/ScriptEngineGetValue.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/ScriptEngineSetValue.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/ScriptEngineEval.sv

TOPLEVEL = DebuggerModuleTestingBRAM
MODULE = test_DebuggerModuleTestingBRAM

include $(shell cocotb-config --makefiles)/Makefile.sim