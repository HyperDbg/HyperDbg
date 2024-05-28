# Makefile

TOPLEVEL_LANG = verilog
VERILOG_SOURCES += $(shell pwd)/../../../generated/DebuggerModuleTestingBRAM.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/InitRegMemFromFile.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/DebuggerMain.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/SendReceiveSynchronizer.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/DebuggerPacketReceiver.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/DebuggerPacketSender.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/DebuggerPacketInterpreter.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/InterpreterSendVersion.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/InterpreterSendError.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/InterpreterPortInformation.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/ScriptExecutionEngine.sv
VERILOG_SOURCES += $(shell pwd)/../../../generated/ScriptEngineEval.sv
TOPLEVEL = DebuggerModuleTestingBRAM
MODULE = test_DebuggerModuleTestingBRAM

include $(shell cocotb-config --makefiles)/Makefile.sim