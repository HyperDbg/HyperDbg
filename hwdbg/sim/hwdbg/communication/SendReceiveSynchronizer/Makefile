# Makefile

TOPLEVEL_LANG = verilog
VERILOG_SOURCES += $(shell pwd)/../../../../generated/SendReceiveSynchronizer.sv
VERILOG_SOURCES += $(shell pwd)/../../../../generated/DebuggerPacketReceiver.sv
VERILOG_SOURCES += $(shell pwd)/../../../../generated/DebuggerPacketSender.sv
TOPLEVEL = SendReceiveSynchronizer
MODULE = test_SendReceiveSynchronizer

include $(shell cocotb-config --makefiles)/Makefile.sim
