import os
import subprocess
import glob

MODELSIM = "/home/sina/intelFPGA/20.1/modelsim_ase/bin"

#
# Check modelsim directory
#
if not os.path.exists(MODELSIM):
    print("[x] Error: The path mdoes not exist")
    exit()
else:
    print("[*] Oh, the modelsim path found :)")

MODELSIM_VCD2WLF = MODELSIM + "/vcd2wlf"
MODELSIM_VSIM = MODELSIM + "/vsim"

#
# Config file variables
#
CONFIG_TEST_MODULE_CLASS = ""
CONFIG_SHOW_ALL_WAVES = True
CONFIG_WAVES_LIST = []

#
# Check if user is root or not
#
if os.geteuid() != 0:
    print("[x] you should run this script with root (sudo) user permission")
    exit()
else:
    print("[*] user is root")

#
# Get the current script's directory
#
current_script_path = os.path.dirname(os.path.abspath(__file__))

WAVE_OUTPUT_FILES_PATH = current_script_path + \
    "/../test_run_dir/DUT_should_pass/"
CONFIG_FILE_PATH = current_script_path + "/modelsim.config"
print("[*] current script path:", WAVE_OUTPUT_FILES_PATH)

#
# Check config file
#
if os.path.exists(CONFIG_FILE_PATH) == False:
    print("[x] config file not found")
    exit()

#
# Interpreting config file
#
with open(CONFIG_FILE_PATH, 'r') as file:
    for line in file:

        if line.lower().startswith("module:") or line.lower().startswith("module :"):
            # it's the test module name
            CONFIG_TEST_MODULE_CLASS = line.split(":")[1]
            print("[*] found module name:", CONFIG_TEST_MODULE_CLASS)
        else:
            # it's a wave, so no longer need to show all waves
            if line.isspace() == False:
                CONFIG_SHOW_ALL_WAVES = False
                CONFIG_WAVES_LIST.append(line)
                print("[*] signal filter for:", line)

#
# Show message if all signals need to be shown
#
if CONFIG_SHOW_ALL_WAVES == True:
    print("[*] no signal filter found, assuming all signals to be shown!")

#
# Check if test module is empty or not
#
if CONFIG_TEST_MODULE_CLASS.isspace() == True:
    print("[x] main test module not found, please add 'module:' to the config file")
    exit()

#
# Set the current working directory
#
os.chdir(current_script_path + "/..")
print("[*] current working directory: " + format(os.getcwd()))

#
# Create TCL config file
#
print("[*] writing to TCL config file: " +
      current_script_path + '/modelsim.tcl')
if CONFIG_SHOW_ALL_WAVES:
    with open(current_script_path + '/modelsim.tcl', 'w') as f:
    	# add the clock at top of the signals by default
        f.write("add wave -position insertpoint clock\n")
        f.write("add wave -position insertpoint *\n")
else:
    with open(current_script_path + '/modelsim.tcl', 'w') as f:
        for item in CONFIG_WAVES_LIST:
            f.write("add wave -position insertpoint {*" + item.replace('\n','').replace('\r', '') + '*}\n')

#
# Remove all the previous *.wlf, *.vcd, *.fir files
#
print("[*] removing previously generated files")
for file_name in os.listdir(WAVE_OUTPUT_FILES_PATH):
    if file_name.endswith('.wlf') or file_name.endswith('.vcd') or file_name.endswith('.fir'):
        os.remove(os.path.join(WAVE_OUTPUT_FILES_PATH, file_name))

#
# Run the VCD wave generator
#
print("[*] running chisel VCD file generator for module: " +
      CONFIG_TEST_MODULE_CLASS)
print("running command: '" + "sbt testOnly " + CONFIG_TEST_MODULE_CLASS + " -- -DwriteVcd=1" + "'")
result = subprocess.run(
    ["sbt", "testOnly " + CONFIG_TEST_MODULE_CLASS + " -- -DwriteVcd=1"], stdout=subprocess.PIPE)
print(result.stdout.decode())

#
# Get all files in directory
#
files = glob.glob(WAVE_OUTPUT_FILES_PATH + "/*")

#
# Sort files by last modified time
#
files.sort(key=lambda x: os.path.getmtime(x))

#
# Check if the list is empty or not
#
if not files or not files[-1].endswith('.vcd') :
    print("[x] there was an error in generating VCD files")
    exit()

#
# Get the latest modified file
#
latest_vcd_file = files[-1]
print("[*] latest generated VCD file: " + latest_vcd_file)

#
# Converting VCD to WLF
#
print("[*] converting VCD file to WLF file")
result = subprocess.run(
    [MODELSIM_VCD2WLF, latest_vcd_file, latest_vcd_file + ".wlf"], stdout=subprocess.PIPE)
print(result.stdout.decode())

#
# Run the generated WLF file
#
print("[*] openning file in vsim: " + latest_vcd_file + ".wlf")
subprocess.run([MODELSIM_VSIM, latest_vcd_file + ".wlf",
               "-do", current_script_path + '/modelsim.tcl'])
