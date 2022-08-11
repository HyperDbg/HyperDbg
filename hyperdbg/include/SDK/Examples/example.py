import ctypes

# Load HyperDbg CTRL into memory

hllDll = ctypes.WinDLL ("build\\bin\\release\\..\\..\\..\\HPRDBGCTRL.dll")

# Set up prototype and parameters for the desired function call
# HLLAPI

ApiProto = ctypes.WINFUNCTYPE (
    ctypes.c_int,      # Return type.
    )
hllApiParams = (1, "p1", 0), (1, "p2", 0), (1, "p3",0), (1, "p4",0),

# Actually map the call ("HLLAPI(...)") to a Python name

hllApi = ApiProto (("HyperDbgInstallVmmDriver", hllDll), hllApiParams)

# This is how you can actually call the DLL function
# Set up the variables and call the Python name with them

p1 = ctypes.c_int (1)
hllApi (ctypes.byref (p1), p2, ctypes.byref (p3), ctypes.byref (p4))
