package sdk

import (
	"encoding/hex"
	"github.com/ddkwork/golibrary/std/stream"
	"strconv"
	"strings"
)

type debugger struct{}

func (debugger) VmxSupportDetection() bool {
	return request[bool]("VmxSupportDetection", nil)
}
func (debugger) CpuReadVendorString(vendor_string string) {
	request[void]("CpuReadVendorString", map[string]string{"vendor_string": vendor_string})
}
func (debugger) LoadVmmModule() int {
	return request[int]("LoadVmmModule", nil)
}
func (debugger) UnloadVmm() int {
	return request[int]("UnloadVmm", nil)
}
func (debugger) InstallVmmDriver() int {
	return request[int]("InstallVmmDriver", nil)
}
func (debugger) UninstallVmmDriver() int {
	return request[int]("UninstallVmmDriver", nil)
}
func (debugger) StopVmmDriver() int {
	return request[int]("StopVmmDriver", nil)
}
func (debugger) Interpreter(command string) int {
	return request[int]("Interpreter", map[string]string{"command": command})
}
func (debugger) TestCommandParser(command string, number_of_tokens uint32, tokens_list []string, failed_token_num uint32, failed_token_position uint32) bool {
	return request[bool]("TestCommandParser", map[string]string{
		"command":               command,
		"number_of_tokens":      strconv.FormatUint(uint64(number_of_tokens), 10),
		"tokens_list":           strings.Join(tokens_list, " "),
		"failed_token_num":      strconv.FormatUint(uint64(failed_token_num), 10),
		"failed_token_position": strconv.FormatUint(uint64(failed_token_position), 10),
	})
}
func (debugger) TestCommandParserShowTokens(command string) {
	request[void]("TestCommandParserShowTokens", map[string]string{"command": command})
}
func (debugger) ShowSignature() {
	request[void]("ShowSignature", nil)
}
func (debugger) SetTextMessageCallback() {
	request[void]("SetTextMessageCallback", nil)
}
func (debugger) SetTextMessageCallbackUsingSharedBuffer() {
	request[void]("SetTextMessageCallbackUsingSharedBuffer", nil)
}
func (debugger) UnsetTextMessageCallback() {
	request[void]("UnsetTextMessageCallback", nil)
}
func (debugger) ScriptReadFileAndExecuteCommandline(argc int, argv string) int {
	return request[int]("ScriptReadFileAndExecuteCommandline", map[string]string{
		"argc": strconv.Itoa(argc),
		"argv": argv,
	})
}
func (debugger) ContinuePreviousCommand() bool {
	return request[bool]("ContinuePreviousCommand", nil)
}
func (debugger) CheckMultilineCommand(current_command string, reset bool) bool {
	return request[bool]("CheckMultilineCommand", map[string]string{
		"current_command": current_command,
		"reset":           strconv.FormatBool(reset),
	})
}
func (debugger) ConnectLocalDebugger() {
	request[void]("ConnectLocalDebugger", nil)
}
func (debugger) ConnectRemoteDebugger(ip string, port string) bool {
	return request[bool]("ConnectRemoteDebugger", map[string]string{
		"ip":   ip,
		"port": port,
	})
}
func (debugger) Continue() {
	request[void]("Continue", nil)
}
func (debugger) Pause() {
	request[void]("Pause", nil)
}
func (debugger) SetBreakPoint(address uint64, pid uint32, tid uint32, core_numer uint32) {
	request[void]("SetBreakPoint", map[string]string{
		"address":    strconv.FormatUint(address, 10),
		"pid":        strconv.FormatUint(uint64(pid), 10),
		"tid":        strconv.FormatUint(uint64(tid), 10),
		"core_numer": strconv.FormatUint(uint64(core_numer), 10),
	})
}
func (debugger) SetCustomDriverPath(driver_file_path string, driver_name string) bool {
	return request[bool]("SetCustomDriverPath", map[string]string{
		"driver_file_path": driver_file_path,
		"driver_name":      driver_name,
	})
}
func (debugger) UseDefaultDriverPath() {
	request[void]("UseDefaultDriverPath", nil)
}
func (debugger) ReadMemory(target_address uint64, memory_type DEBUGGER_READ_MEMORY_TYPE, reading_Type DEBUGGER_READ_READING_TYPE, pid uint32, size uint32, get_address_mode bool, address_mode DEBUGGER_READ_MEMORY_ADDRESS_MODE, target_buffer_to_store []byte, return_length uint32) bool {
	return request[bool]("ReadMemory", map[string]string{
		"target_address":         strconv.FormatUint(target_address, 10),
		"memory_type":            strconv.FormatUint(uint64(memory_type), 10),
		"reading_Type":           strconv.FormatUint(uint64(reading_Type), 10),
		"pid":                    strconv.FormatUint(uint64(pid), 10),
		"size":                   strconv.FormatUint(uint64(size), 10),
		"get_address_mode":       strconv.FormatBool(get_address_mode),
		"address_mode":           strconv.FormatUint(uint64(address_mode), 10),
		"target_buffer_to_store": hex.EncodeToString(target_buffer_to_store),
		"return_length":          strconv.FormatUint(uint64(return_length), 10),
	})
}
func (debugger) ShowMemoryOrDisassemble(style DEBUGGER_SHOW_MEMORY_STYLE, address uint64, memory_type DEBUGGER_READ_MEMORY_TYPE, reading_type DEBUGGER_READ_READING_TYPE, pid uint32, size uint32, dt_details DEBUGGER_DT_COMMAND_OPTIONS) {
	request[void]("ShowMemoryOrDisassemble", map[string]string{
		"style":        strconv.FormatUint(uint64(style), 10),
		"address":      strconv.FormatUint(address, 10),
		"memory_type":  strconv.FormatUint(uint64(memory_type), 10),
		"reading_type": strconv.FormatUint(uint64(reading_type), 10),
		"pid":          strconv.FormatUint(uint64(pid), 10),
		"size":         strconv.FormatUint(uint64(size), 10),
		"dt_details":   string(stream.MarshalJSON(dt_details)),
	})
}
func (debugger) ReadAllRegisters(guest_registers GUEST_REGS, extra_registers GUEST_EXTRA_REGISTERS) bool {
	return request[bool]("ReadAllRegisters", map[string]string{
		"guest_registers": string(stream.MarshalJSON(guest_registers)),
		"extra_registers": string(stream.MarshalJSON(extra_registers)),
	})
}
func (debugger) ReadTargetRegister(register_id REGS_ENUM, target_register uint64) bool {
	return request[bool]("ReadTargetRegister", map[string]string{
		"register_id":     strconv.FormatUint(uint64(register_id), 10),
		"target_register": strconv.FormatUint(uint64(target_register), 10),
	})
}
func (debugger) WriteTargetRegister(register_id REGS_ENUM, value uint64) bool {
	return request[bool]("WriteTargetRegister", map[string]string{
		"register_id": strconv.FormatUint(uint64(register_id), 10),
		"value":       strconv.FormatUint(value, 10),
	})
}
func (debugger) RegisterShowAll() bool {
	return request[bool]("RegisterShowAll", nil)
}
func (debugger) RegisterShowTargetRegister(register_id REGS_ENUM) bool {
	return request[bool]("RegisterShowTargetRegister", map[string]string{"register_id": strconv.FormatUint(uint64(register_id), 10)})
}
func (debugger) WriteMemory(destination_address uint64, memory_type DEBUGGER_EDIT_MEMORY_TYPE, process_id uint32, source_address uint64, number_of_bytes uint32) bool {
	return request[bool]("WriteMemory", map[string]string{
		"destination_address": strconv.FormatUint(uint64(destination_address), 10),
		"memory_type":         strconv.FormatUint(uint64(memory_type), 10),
		"process_id":          strconv.FormatUint(uint64(process_id), 10),
		"source_address":      strconv.FormatUint(uint64(source_address), 10),
		"number_of_bytes":     strconv.FormatUint(uint64(number_of_bytes), 10),
	})
}
func (debugger) DebuggerGetKernelBase() uint64 {
	return request[uint64]("DebuggerGetKernelBase", nil)
}
func (debugger) DebugRemoteDeviceUsingComPort(port_name string, baudrate uint32, pause_after_connection bool) bool {
	return request[bool]("DebugRemoteDeviceUsingComPort", map[string]string{
		"port_name":              port_name,
		"baudrate":               strconv.FormatUint(uint64(baudrate), 10),
		"pause_after_connection": strconv.FormatBool(pause_after_connection),
	})
}
func (debugger) DebugRemoteDeviceUsingNamedPipe(named_pipe string, pause_after_connection bool) bool {
	return request[bool]("DebugRemoteDeviceUsingNamedPipe", map[string]string{
		"named_pipe":             named_pipe,
		"pause_after_connection": strconv.FormatBool(pause_after_connection),
	})
}
func (debugger) DebugCloseRemoteDebugger() bool {
	return request[bool]("DebugCloseRemoteDebugger", nil)
}
func (debugger) DebugCurrentDeviceUsingComPort(port_name string, baudrate uint32) bool {
	return request[bool]("DebugCurrentDeviceUsingComPort", map[string]string{
		"port_name": port_name,
		"baudrate":  strconv.FormatUint(uint64(baudrate), 10),
	})
}
func (debugger) StartProcess(path string) bool {
	return request[bool]("StartProcess", map[string]string{"path": path})
}
func (debugger) StartProcessWithArgs(path string, arguments string) bool {
	return request[bool]("StartProcessWithArgs", map[string]string{
		"path":      path,
		"arguments": arguments,
	})
}
func (debugger) AssembleGetLength(assembly_code string, start_address uint64, length uint32) bool {
	return request[bool]("AssembleGetLength", map[string]string{
		"assembly_code": assembly_code,
		"start_address": strconv.FormatUint(start_address, 10),
		"length":        strconv.FormatUint(uint64(length), 10),
	})
}
func (debugger) Assemble(assembly_code string, start_address uint64, buffer_to_store_assembled_data uint64, buffer_size uint32) bool {
	return request[bool]("Assemble", map[string]string{
		"assembly_code":                  assembly_code,
		"start_address":                  strconv.FormatUint(start_address, 10),
		"buffer_to_store_assembled_data": strconv.FormatUint(uint64(buffer_to_store_assembled_data), 10),
		"buffer_size":                    strconv.FormatUint(uint64(buffer_size), 10),
	})
}
func (debugger) SetupPathForFileName(filename string, file_location string, buffer_len uint32, check_file_existence bool) bool {
	return request[bool]("SetupPathForFileName", map[string]string{
		"filename":             filename,
		"file_location":        file_location,
		"buffer_len":           strconv.FormatUint(uint64(buffer_len), 10),
		"check_file_existence": strconv.FormatBool(check_file_existence),
	})
}
func (debugger) SteppingInstrumentationStepIn() bool {
	return request[bool]("SteppingInstrumentationStepIn", nil)
}
func (debugger) SteppingRegularStepIn() bool {
	return request[bool]("SteppingRegularStepIn", nil)
}
func (debugger) SteppingStepOver() bool {
	return request[bool]("SteppingStepOver", nil)
}
func (debugger) SteppingInstrumentationStepInForTracking() bool {
	return request[bool]("SteppingInstrumentationStepInForTracking", nil)
}
func (debugger) SteppingStepOverForGu(last_instruction bool) bool {
	return request[bool]("SteppingStepOverForGu", map[string]string{"last_instruction": strconv.FormatBool(last_instruction)})
}
func (debugger) GetLocalApic(local_apic PLAPIC_PAGE, is_using_x2apic bool) bool {
	return request[bool]("GetLocalApic", map[string]string{
		"local_apic":      string(stream.MarshalJSON(local_apic)),
		"is_using_x2apic": strconv.FormatBool(is_using_x2apic),
	})
}
func (debugger) GetIoApic(io_apic IO_APIC_ENTRY_PACKETS) bool {
	return request[bool]("GetIoApic", map[string]string{"io_apic": string(stream.MarshalJSON(io_apic))})
}
func (debugger) GetIdtEntry(idt_packet INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS) bool {
	return request[bool]("GetIdtEntry", map[string]string{"idt_packet": string(stream.MarshalJSON(idt_packet))})
}
func (debugger) HwdbgScriptRunScript(script string, instance_filepath_to_read string, hardware_script_file_path_to_save string, initial_bram_buffer_size uint32) bool {
	return request[bool]("HwdbgScriptRunScript", map[string]string{
		"script":                            script,
		"instance_filepath_to_read":         instance_filepath_to_read,
		"hardware_script_file_path_to_save": hardware_script_file_path_to_save,
		"initial_bram_buffer_size":          strconv.FormatUint(uint64(initial_bram_buffer_size), 10),
	})
}
func (debugger) ScriptEngineWrapperTestParserForHwdbg(Expr string) {
	request[void]("ScriptEngineWrapperTestParserForHwdbg", map[string]string{"Expr": Expr})
}
func (debugger) EnableTransparentMode(ProcessId uint32, ProcessName string, IsProcessId bool) bool {
	return request[bool]("EnableTransparentMode", map[string]string{
		"ProcessId":   strconv.FormatUint(uint64(ProcessId), 10),
		"ProcessName": ProcessName,
		"IsProcessId": strconv.FormatBool(IsProcessId),
	})
}
func (debugger) DisableTransparentMode() bool {
	return request[bool]("DisableTransparentMode", nil)
}
