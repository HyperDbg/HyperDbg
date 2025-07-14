import sys
import requests
import json

from mcp.server.fastmcp import FastMCP

DEFAULT_HyperDBG_SERVER = "http://127.0.0.1:8888/"
hyperdbg_server_url = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_HyperDBG_SERVER

mcp = FastMCP("hyperdbg-mcp")

def safe_get(endpoint: str, params: dict = None):
    """
    Perform a GET request with optional query parameters.
    Returns parsed JSON if possible, otherwise text content
    """
    if params is None:
        params = {}

    url = f"{hyperdbg_server_url}{endpoint}"

    try:
        response = requests.get(url, params=params, timeout=15)
        response.encoding = 'utf-8'
        if response.ok:
            # Try to parse as JSON first
            try:
                return response.json()
            except ValueError:
                return response.text.strip()
        else:
            return f"Error {response.status_code}: {response.text.strip()}"
    except Exception as e:
        return f"Request failed: {str(e)}"

def safe_post(endpoint: str, data: dict | str):
    """
    Perform a POST request with data.
    Returns parsed JSON if possible, otherwise text content
    """
    try:
        url = f"{hyperdbg_server_url}{endpoint}"
        if isinstance(data, dict):
            response = requests.post(url, data=data, timeout=5)
        else:
            response = requests.post(url, data=data.encode("utf-8"), timeout=5)
        
        response.encoding = 'utf-8'
        
        if response.ok:
            # Try to parse as JSON first
            try:
                return response.json()
            except ValueError:
                return response.text.strip()
        else:
            return f"Error {response.status_code}: {response.text.strip()}"
    except Exception as e:
        return f"Request failed: {str(e)}"
@mcp.tool()
def VmxSupportDetection()

@mcp.tool()
def CpuReadVendorString()

@mcp.tool()
def HyperDbgLoadVmmModule()

@mcp.tool()
def HyperDbgUnloadVmm()

@mcp.tool()
def HyperDbgInstallVmmDriver()

@mcp.tool()
def HyperDbgUninstallVmmDriver()

@mcp.tool()
def HyperDbgStopVmmDriver()

@mcp.tool()
def HyperDbgInterpreter()

@mcp.tool()
def HyperDbgTestCommandParser()

@mcp.tool()
def HyperDbgTestCommandParserShowTokens()

@mcp.tool()
def HyperDbgShowSignature()

@mcp.tool()
def SetTextMessageCallback()

@mcp.tool()
def SetTextMessageCallbackUsingSharedBuffer()

@mcp.tool()
def UnsetTextMessageCallback()

@mcp.tool()
def ScriptReadFileAndExecuteCommandline()

@mcp.tool()
def ContinuePreviousCommand()

@mcp.tool()
def CheckMultilineCommand()

@mcp.tool()
def ConnectLocalDebugger()

@mcp.tool()
def ConnectRemoteDebugger()

@mcp.tool()
def Continue()

@mcp.tool()
def Pause()

@mcp.tool()
def SetBreakPoint()

@mcp.tool()
def SetCustomDriverPath()

@mcp.tool()
def UseDefaultDriverPath()

@mcp.tool()
def HyperDbgReadMemory()

@mcp.tool()
def HyperDbgShowMemoryOrDisassemble()

@mcp.tool()
def HyperDbgReadAllRegisters()

@mcp.tool()
def HyperDbgReadTargetRegister()

@mcp.tool()
def HyperDbgWriteTargetRegister()

@mcp.tool()
def HyperDbgRegisterShowAll()

@mcp.tool()
def HyperDbgRegisterShowTargetRegister()

@mcp.tool()
def HyperDbgWriteMemory()

@mcp.tool()
def DebuggerGetKernelBase()

@mcp.tool()
def HyperDbgDebugRemoteDeviceUsingComPort()

@mcp.tool()
def HyperDbgDebugRemoteDeviceUsingNamedPipe()

@mcp.tool()
def HyperDbgDebugCloseRemoteDebugger()

@mcp.tool()
def HyperDbgDebugCurrentDeviceUsingComPort()

@mcp.tool()
def StartProcess()

@mcp.tool()
def StartProcessWithArgs()

@mcp.tool()
def HyperDbgAssembleGetLength()

@mcp.tool()
def HyperDbgAssemble()

@mcp.tool()
def SetupPathForFileName()

@mcp.tool()
def SteppingInstrumentationStepIn()

@mcp.tool()
def SteppingRegularStepIn()

@mcp.tool()
def SteppingStepOver()

@mcp.tool()
def SteppingInstrumentationStepInForTracking()

@mcp.tool()
def SteppingStepOverForGu()

@mcp.tool()
def HyperDbgGetLocalApic()

@mcp.tool()
def HyperDbgGetIoApic()

@mcp.tool()
def HyperDbgGetIdtEntry()

@mcp.tool()
def HwdbgScriptRunScript()

@mcp.tool()
def ScriptEngineWrapperTestParserForHwdbg()

@mcp.tool()
def HyperDbgEnableTransparentMode()

@mcp.tool()
def HyperDbgDisableTransparentMode()

if __name__ == "__main__":
    mcp.run()
