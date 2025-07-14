package sdk

import (
	"cmp"
	"encoding/hex"
	"fmt"
	"io"
	"net/http"
	"strconv"
	"strings"
	"time"

	"github.com/ddkwork/golibrary/std/mylog"
)

const DefaultHyperdbgServer = "http://127.0.0.1:8888/"

var client = &http.Client{
	Timeout: 15 * time.Second,
	Transport: &http.Transport{
		DisableKeepAlives: true,
	},
}

//	Type type Ordered interface {
//		~int | ~int8 | ~int16 | ~int32 | ~int64 |
//			~uint | ~uint8 | ~uint16 | ~uint32 | ~uint64 | ~uintptr |
//			~float32 | ~float64 |
//			~string
//	}
type Type interface {
	cmp.Ordered |
		bool |
		[]byte |
		void
	//moduleInfo |
	//[]moduleInfo |
	//moduleSectionInfo |
	//[]moduleSectionInfo |
	//moduleExport |
	//[]moduleExport |
	//moduleImport |
	//[]moduleImport |
	//memoryBase |
	//disassemblerAddress |
	//disassembleRip |
	//disassembleRipWithSetupIn |
	//assemblerResult |
}
type void any

func request[T Type](endpoint string, params map[string]string) T {
	x64dbgServerURL := DefaultHyperdbgServer
	url := x64dbgServerURL + endpoint

	// 添加查询参数
	if len(params) > 0 {
		query := ""
		for key, value := range params {
			query += key + "=" + value + "&"
		}
		url += "?" + strings.TrimSuffix(query, "&")
	}

	resp := mylog.Check2(client.Get(url))
	defer func() {
		mylog.Check(resp.Body.Close())
	}()

	body := mylog.Check2(io.ReadAll(resp.Body))

	if resp.StatusCode != http.StatusOK {
		mylog.Check(fmt.Sprintf("Error %d: %s", resp.StatusCode, string(body)))
	}

	str := strings.TrimSpace(string(body))
	base := 10
	if strings.HasPrefix(str, "0x") {
		base = 16
	}
	str = strings.TrimPrefix(str, "0x")
	var zero T
	switch v := any(zero).(type) {
	case bool:
		if strings.EqualFold(str, "true") {
			return any(true).(T)
		}
		if strings.EqualFold(str, "false") {
			return any(false).(T)
		}
	case []byte:
		b := mylog.Check2(hex.DecodeString(str))
		return any(b).(T)
	case int:

		value := mylog.Check2(strconv.ParseInt(str, base, 64))
		return any(value).(T)
	case int8:

		value := mylog.Check2(strconv.ParseInt(str, base, 8))
		return any(value).(T)
	case int16:

		value := mylog.Check2(strconv.ParseInt(str, base, 16))
		return any(value).(T)
	case int32:

		value := mylog.Check2(strconv.ParseInt(str, base, 32))
		return any(value).(T)
	case int64:

		value := mylog.Check2(strconv.ParseInt(str, base, 64))
		return any(value).(T)
	case uint:

		value := mylog.Check2(strconv.ParseUint(str, base, 64))
		return any(value).(T)
	case uint8:

		value := mylog.Check2(strconv.ParseUint(str, base, 8))
		return any(value).(T)
	case uint16:

		value := mylog.Check2(strconv.ParseUint(str, base, 16))
		return any(value).(T)
	case uint32:

		value := mylog.Check2(strconv.ParseUint(str, base, 32))
		return any(value).(T)
	case uint64:

		value := mylog.Check2(strconv.ParseUint(str, base, 64))
		return any(value).(T)
	case uintptr:

		value := mylog.Check2(strconv.ParseUint(str, base, 64))
		return any(value).(T)
	case float32:
		value := mylog.Check2(strconv.ParseFloat(str, 32))
		return any(value).(T)
	case float64:
		value := mylog.Check2(strconv.ParseFloat(str, 64))
		return any(value).(T)
	case string:
		return any(str).(T)

		//todo 处理cpp服务端的字段返回 0x12345678 这种格式，我估计json会解码失败
	//case moduleInfo:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case []moduleInfo:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case moduleSectionInfo:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case []moduleSectionInfo:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case moduleExport:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case []moduleExport:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case moduleImport:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case []moduleImport:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case memoryBase:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case disassemblerAddress:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case disassembleRip:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case disassembleRipWithSetupIn:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	//case assemblerResult:
	//	mylog.Check(json.Unmarshal(body, &v))
	//	return any(v).(T)
	case void:
		v = v
		return any(nil).(T)

	}
	panic("not support type")
}

type (
	DEBUGGER_READ_MEMORY_ADDRESS_MODE byte
	DEBUGGER_READ_MEMORY_TYPE         byte
	DEBUGGER_EDIT_MEMORY_TYPE         byte
	DEBUGGER_READ_READING_TYPE        byte
	REGS_ENUM                         byte
	DEBUGGER_SHOW_MEMORY_STYLE        byte
)

const (
	DEBUGGER_SHOW_COMMAND_DT DEBUGGER_SHOW_MEMORY_STYLE = iota + 1
	DEBUGGER_SHOW_COMMAND_DISASSEMBLE64
	DEBUGGER_SHOW_COMMAND_DISASSEMBLE32
	DEBUGGER_SHOW_COMMAND_DB
	DEBUGGER_SHOW_COMMAND_DC
	DEBUGGER_SHOW_COMMAND_DQ
	DEBUGGER_SHOW_COMMAND_DD
	DEBUGGER_SHOW_COMMAND_DUMP
)

const (
	READ_FROM_KERNEL DEBUGGER_READ_READING_TYPE = iota
	READ_FROM_VMX_ROOT
)

const (
	EDIT_VIRTUAL_MEMORY DEBUGGER_EDIT_MEMORY_TYPE = iota
	EDIT_PHYSICAL_MEMORY
)

const (
	DEBUGGER_READ_PHYSICAL_ADDRESS DEBUGGER_READ_MEMORY_TYPE = iota
	DEBUGGER_READ_VIRTUAL_ADDRESS
)

const (
	DEBUGGER_READ_ADDRESS_MODE_32_BIT DEBUGGER_READ_MEMORY_ADDRESS_MODE = iota
	DEBUGGER_READ_ADDRESS_MODE_64_BIT
)
const MAX_NUMBER_OF_IDT_ENTRIES = 256
const MAX_NUMBER_OF_IO_APIC_ENTRIES = 400

type (
	INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS struct {
		KernelStatus uint32
		IdtEntry     [MAX_NUMBER_OF_IDT_ENTRIES]uint64
	}

	IO_APIC_ENTRY_PACKETS struct {
		ApicBasePa uint64
		ApicBaseVa uint64
		IoIdReg    uint32
		IoLl       uint32
		IoArbIdReg uint32
		LlLhData   [MAX_NUMBER_OF_IO_APIC_ENTRIES]uint64
	}

	DEBUGGER_DT_COMMAND_OPTIONS struct {
		TypeName             string
		SizeOfTypeName       uint64
		Address              uint64
		IsStruct             bool
		BufferAddress        uintptr
		TargetPid            uint32
		AdditionalParameters string
	}

	GUEST_REGS struct { //todo export for json marshal
		rax uint64
		rcx uint64
		rdx uint64
		rbx uint64
		rsp uint64
		rbp uint64
		rsi uint64
		rdi uint64
		r8  uint64
		r9  uint64
		r10 uint64
		r11 uint64
		r12 uint64
		r13 uint64
		r14 uint64
		r15 uint64
	}

	GUEST_EXTRA_REGISTERS struct {
		CS     uint16
		DS     uint16
		FS     uint16
		GS     uint16
		ES     uint16
		SS     uint16
		RFLAGS uint64
		RIP    uint64
	}
)

// LAPIC_PAGE 表示本地高级可编程中断控制器(LAPIC)的寄存器页面布局
// 总大小固定为 0x400 字节 (1024 字节)
type LAPIC_PAGE struct {
	Reserved000             [0x10]byte // 偏移 0x00-0x0F
	Reserved010             [0x10]byte // 偏移 0x10-0x1F
	Id                      uint32     // 偏移 0x020 - APIC ID
	Reserved024             [0x0C]byte // 偏移 0x024-0x02F
	Version                 uint32     // 偏移 0x030 - 版本号
	Reserved034             [0x0C]byte // 偏移 0x034-0x03F
	Reserved040             [0x40]byte // 偏移 0x040-0x07F
	TPR                     uint32     // 偏移 0x080 - 任务优先级
	Reserved084             [0x0C]byte // 偏移 0x084-0x08F
	ArbitrationPriority     uint32     // 偏移 0x090 - 仲裁优先级
	Reserved094             [0x0C]byte // 偏移 0x094-0x09F
	ProcessorPriority       uint32     // 偏移 0x0A0 - 处理器优先级
	Reserved0A4             [0x0C]byte // 偏移 0x0A4-0x0AF
	EOI                     uint32     // 偏移 0x0B0 - 中断结束寄存器
	Reserved0B4             [0x0C]byte // 偏移 0x0B4-0x0BF
	RemoteRead              uint32     // 偏移 0x0C0 - 远程读寄存器
	Reserved0C4             [0x0C]byte // 偏移 0x0C4-0x0CF
	LogicalDestination      uint32     // 偏移 0x0D0 - 逻辑目标寄存器
	Reserved0D4             [0x0C]byte // 偏移 0x0D4-0x0DF
	DestinationFormat       uint32     // 偏移 0x0E0 - 目标格式寄存器
	Reserved0E4             [0x0C]byte // 偏移 0x0E4-0x0EF
	SpuriousInterruptVector uint32     // 偏移 0x0F0 - 伪中断向量
	Reserved0F4             [0x0C]byte // 偏移 0x0F4-0x0FF
	ISR                     [8]uint32  // 偏移 0x100-0x13F (实际32位数组，分8个32位组存储)
	//_                           [0x40]byte   // 对齐填充 0x140-0x17F
	TMR [8]uint32 // 偏移 0x180-0x1BF (实际32位数组，分8个32位组存储)
	//_                           [0x40]byte   // 对齐填充 0x1C0-0x1FF
	IRR [8]uint32 // 偏移 0x200-0x23F (实际32位数组，分8个32位组存储)
	//_                           [0x40]byte   // 对齐填充 0x240-0x27F
	ErrorStatus         uint32     // 偏移 0x280 - 错误状态寄存器
	Reserved284         [0x0C]byte // 偏移 0x284-0x28F
	Reserved290         [0x60]byte // 偏移 0x290-0x2EF
	LvtCmci             uint32     // 偏移 0x2F0 - CMCI中断向量
	Reserved2F4         [0x0C]byte // 偏移 0x2F4-0x2FF
	IcrLow              uint32     // 偏移 0x300 - 中断命令寄存器低32位
	Reserved304         [0x0C]byte // 偏移 0x304-0x30F
	IcrHigh             uint32     // 偏移 0x310 - 中断命令寄存器高32位
	Reserved314         [0x0C]byte // 偏移 0x314-0x31F
	LvtTimer            uint32     // 偏移 0x320 - 定时器中断向量
	Reserved324         [0x0C]byte // 偏移 0x324-0x32F
	LvtThermalSensor    uint32     // 偏移 0x330 - 热传感器中断向量
	Reserved334         [0x0C]byte // 偏移 0x334-0x33F
	LvtPerfMonCounters  uint32     // 偏移 0x340 - 性能监控计数器中断向量
	Reserved344         [0x0C]byte // 偏移 0x344-0x34F
	LvtLINT0            uint32     // 偏移 0x350 - LINT0中断向量
	Reserved354         [0x0C]byte // 偏移 0x354-0x35F
	LvtLINT1            uint32     // 偏移 0x360 - LINT1中断向量
	Reserved364         [0x0C]byte // 偏移 0x364-0x36F
	LvtError            uint32     // 偏移 0x370 - 错误中断向量
	Reserved374         [0x0C]byte // 偏移 0x374-0x37F
	InitialCount        uint32     // 偏移 0x380 - 初始计数寄存器
	Reserved384         [0x0C]byte // 偏移 0x384-0x38F
	CurrentCount        uint32     // 偏移 0x390 - 当前计数寄存器
	Reserved394         [0x0C]byte // 偏移 0x394-0x39F
	Reserved3A0         [0x40]byte // 偏移 0x3A0-0x3DF
	DivideConfiguration uint32     // 偏移 0x3E0 - 分频配置寄存器
	Reserved3E4         [0x0C]byte // 偏移 0x3E4-0x3EF
	SelfIpi             uint32     // 偏移 0x3F0 - 自中断寄存器(X2APIC)
	Reserved3F4         [0x0C]byte // 偏移 0x3F4-0x3FF(X2APIC保留)
}

// PLAPIC_PAGE 是指向 LAPIC_PAGE 的指针类型
type PLAPIC_PAGE *LAPIC_PAGE

const (
	REGISTER_RAX REGS_ENUM = iota
	REGISTER_EAX
	REGISTER_AX
	REGISTER_AH
	REGISTER_AL
	REGISTER_RCX
	REGISTER_ECX
	REGISTER_CX
	REGISTER_CH
	REGISTER_CL
	REGISTER_RDX
	REGISTER_EDX
	REGISTER_DX
	REGISTER_DH
	REGISTER_DL
	REGISTER_RBX
	REGISTER_EBX
	REGISTER_BX
	REGISTER_BH
	REGISTER_BL
	REGISTER_RSP
	REGISTER_ESP
	REGISTER_SP
	REGISTER_SPL
	REGISTER_RBP
	REGISTER_EBP
	REGISTER_BP
	REGISTER_BPL
	REGISTER_RSI
	REGISTER_ESI
	REGISTER_SI
	REGISTER_SIL
	REGISTER_RDI
	REGISTER_EDI
	REGISTER_DI
	REGISTER_DIL
	REGISTER_R8
	REGISTER_R8D
	REGISTER_R8W
	REGISTER_R8H
	REGISTER_R8L
	REGISTER_R9
	REGISTER_R9D
	REGISTER_R9W
	REGISTER_R9H
	REGISTER_R9L
	REGISTER_R10
	REGISTER_R10D
	REGISTER_R10W
	REGISTER_R10H
	REGISTER_R10L
	REGISTER_R11
	REGISTER_R11D
	REGISTER_R11W
	REGISTER_R11H
	REGISTER_R11L
	REGISTER_R12
	REGISTER_R12D
	REGISTER_R12W
	REGISTER_R12H
	REGISTER_R12L
	REGISTER_R13
	REGISTER_R13D
	REGISTER_R13W
	REGISTER_R13H
	REGISTER_R13L
	REGISTER_R14
	REGISTER_R14D
	REGISTER_R14W
	REGISTER_R14H
	REGISTER_R14L
	REGISTER_R15
	REGISTER_R15D
	REGISTER_R15W
	REGISTER_R15H
	REGISTER_R15L
	REGISTER_DS
	REGISTER_ES
	REGISTER_FS
	REGISTER_GS
	REGISTER_CS
	REGISTER_SS
	REGISTER_RFLAGS
	REGISTER_EFLAGS
	REGISTER_FLAGS
	REGISTER_CF
	REGISTER_PF
	REGISTER_AF
	REGISTER_ZF
	REGISTER_SF
	REGISTER_TF
	REGISTER_IF
	REGISTER_DF
	REGISTER_OF
	REGISTER_IOPL
	REGISTER_NT
	REGISTER_RF
	REGISTER_VM
	REGISTER_AC
	REGISTER_VIF
	REGISTER_VIP
	REGISTER_ID
	REGISTER_RIP
	REGISTER_EIP
	REGISTER_IP
	REGISTER_IDTR
	REGISTER_LDTR
	REGISTER_GDTR
	REGISTER_TR
	REGISTER_CR0
	REGISTER_CR2
	REGISTER_CR3
	REGISTER_CR4
	REGISTER_CR8
	REGISTER_DR0
	REGISTER_DR1
	REGISTER_DR2
	REGISTER_DR3
	REGISTER_DR6
	REGISTER_DR7
)

var RegistersNames = []string{
	"rax", "eax", "ax", "ah", "al", "rcx", "ecx", "cx",
	"ch", "cl", "rdx", "edx", "dx", "dh", "dl", "rbx",
	"ebx", "bx", "bh", "bl", "rsp", "esp", "sp", "spl",
	"rbp", "ebp", "bp", "bpl", "rsi", "esi", "si", "sil",
	"rdi", "edi", "di", "dil", "r8", "r8d", "r8w", "r8h",
	"r8l", "r9", "r9d", "r9w", "r9h", "r9l", "r10", "r10d",
	"r10w", "r10h", "r10l", "r11", "r11d", "r11w", "r11h", "r11l",
	"r12", "r12d", "r12w", "r12h", "r12l", "r13", "r13d", "r13w",
	"r13h", "r13l", "r14", "r14d", "r14w", "r14h", "r14l", "r15",
	"r15d", "r15w", "r15h", "r15l", "ds", "es", "fs", "gs",
	"cs", "ss", "rflags", "eflags", "flags", "cf", "pf", "af",
	"zf", "sf", "tf", "if", "df", "of", "iopl", "nt",
	"rf", "vm", "ac", "vif", "vip", "id", "rip", "eip",
	"ip", "idtr", "ldtr", "gdtr", "tr", "cr0", "cr2", "cr3",
	"cr4", "cr8", "dr0", "dr1", "dr2", "dr3", "dr6", "dr7",
}
