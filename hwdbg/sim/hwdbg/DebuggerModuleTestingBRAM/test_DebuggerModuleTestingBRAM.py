##
# @file test_DebuggerModuleTestingBRAM.py
#
# @author Sina Karvandi (sina@hyperdbg.org)
#
# @brief Testing module for DebuggerModuleTestingBRAM
#
# @details
#
# @version 0.1
#
# @date 2024-04-21
#
# @copyright This project is released under the GNU Public License v3.
#

import random
import re

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import Timer
from cocotb.types import LogicArray

maximum_number_of_clock_cycles = 1000

'''
  input  clock,
         reset,
         io_en,
         io_inputPin_0,
         io_inputPin_1,
         io_inputPin_2,
         io_inputPin_3,
         io_inputPin_4,
         io_inputPin_5,
         io_inputPin_6,
         io_inputPin_7,
         io_inputPin_8,
         io_inputPin_9,
         io_inputPin_10,
         io_inputPin_11,
         io_inputPin_12,
         io_inputPin_13,
         io_inputPin_14,
         io_inputPin_15,
         io_inputPin_16,
         io_inputPin_17,
         io_inputPin_18,
         io_inputPin_19,
         io_inputPin_20,
         io_inputPin_21,
         io_inputPin_22,
         io_inputPin_23,
         io_inputPin_24,
         io_inputPin_25,
         io_inputPin_26,
         io_inputPin_27,
         io_inputPin_28,
         io_inputPin_29,
         io_inputPin_30,
         io_inputPin_31,
  output io_outputPin_0,
         io_outputPin_1,
         io_outputPin_2,
         io_outputPin_3,
         io_outputPin_4,
         io_outputPin_5,
         io_outputPin_6,
         io_outputPin_7,
         io_outputPin_8,
         io_outputPin_9,
         io_outputPin_10,
         io_outputPin_11,
         io_outputPin_12,
         io_outputPin_13,
         io_outputPin_14,
         io_outputPin_15,
         io_outputPin_16,
         io_outputPin_17,
         io_outputPin_18,
         io_outputPin_19,
         io_outputPin_20,
         io_outputPin_21,
         io_outputPin_22,
         io_outputPin_23,
         io_outputPin_24,
         io_outputPin_25,
         io_outputPin_26,
         io_outputPin_27,
         io_outputPin_28,
         io_outputPin_29,
         io_outputPin_30,
         io_outputPin_31,
  input  io_plInSignal,
  output io_psOutInterrupt
'''

#
# Define a function to extract the numeric part of the string
#
def extract_number(s):
    return int(s.split('_')[1])

def print_bram_content(dut):
    """Printing contents of Block RAM and saving them to a file"""

    #
    # Print the instances and signals (which includes the ports) of the design's toplevel
    #
    print("===================================================================")
    # print("Onstances and signals (which includes the ports) of the design's toplevel:")
    # print(dir(dut))
    # print("===================================================================")

    #
    # Print the instances and signals of "inst_sub_block" under the toplevel
    # which is the instance name of a Verilog module or VHDL entity/component
    #
    # print("Onstances and signals of 'dataOut_initRegMemFromFileModule' under the toplevel:")
    # print(dir(dut.dataOut_initRegMemFromFileModule))

    items_inside_bram_emulator = dir(dut.dataOut_initRegMemFromFileModule)
    mem_items = []

    for item in items_inside_bram_emulator:
        if item.startswith("mem_"):
            mem_items.append(item)

    #
    # Sort the list using the custom key function
    #
    sorted_list = sorted(mem_items, key=extract_number)

    with open("script_buffer_response.txt", "w") as file:
    # with open("bram_instance_info.txt", "w") as file:
        file.write("Content of BRAM after emulation:\n")
        print("Content of BRAM after emulation:")

        #
        # The second half of the BRAM is used for PL to PS communication
        #
        address_of_ps_to_pl_communication = "mem_0"
        address_of_ps_to_pl_communication_checksum1 = "mem_0"
        address_of_ps_to_pl_communication_checksum2 = "mem_1"
        address_of_ps_to_pl_communication_indicator1 = "mem_2"
        address_of_ps_to_pl_communication_indicator2 = "mem_3"
        address_of_ps_to_pl_communication_type_of_packet = "mem_4"
        address_of_ps_to_pl_communication_requested_action_of_the_packet = "mem_5"
        address_of_ps_to_pl_communication_start_of_data = "mem_6"
        
        len_of_sorted_list_div_by_2 = int(len(sorted_list) / 2)
        address_of_pl_to_ps_communication = "mem_" + str(len_of_sorted_list_div_by_2)
        address_of_pl_to_ps_communication_checksum1 = "mem_" + str(len_of_sorted_list_div_by_2 + 0)
        address_of_pl_to_ps_communication_checksum2 = "mem_" + str(len_of_sorted_list_div_by_2 + 1)
        address_of_pl_to_ps_communication_indicator1 = "mem_" + str(len_of_sorted_list_div_by_2 + 2)
        address_of_pl_to_ps_communication_indicator2 = "mem_" + str(len_of_sorted_list_div_by_2 + 3)
        address_of_pl_to_ps_communication_type_of_packet = "mem_" + str(len_of_sorted_list_div_by_2 + 4)
        address_of_pl_to_ps_communication_requested_action_of_the_packet = "mem_" + str(len_of_sorted_list_div_by_2 + 5)
        address_of_pl_to_ps_communication_start_of_data = "mem_" + str(len_of_sorted_list_div_by_2 + 6)

        print("Address of PL to PS communication: " + address_of_pl_to_ps_communication)

        for item in sorted_list:
            element = getattr(dut.dataOut_initRegMemFromFileModule, item)

            #
            # Print the target register in binary format
            #
            # print(str(element))

            #
            # Convert binary to int
            #
            int_content = int(str(element.value), 2)

            #
            # Convert integer to hexadecimal string with at least 8 characters
            #
            hex_string = f'{int_content:08x}'

            final_string = ""
            if len(item) == 5:
                final_string = item + ":   " + hex_string
            elif len(item) == 6:
                final_string = item + ":  " + hex_string
            else:
                final_string = item + ": " + hex_string

            #
            # Make a separation between PS and PL area
            #
            if item == address_of_ps_to_pl_communication:
                file.write("\nPS to PL area:\n")
                print("\nPS to PL area:")
            elif item == address_of_pl_to_ps_communication:
                file.write("\nPL to PS area:\n")
                print("\nPL to PS area:")
            
            if item == address_of_ps_to_pl_communication_checksum1 or \
            item == address_of_ps_to_pl_communication_checksum2 or \
            item == address_of_pl_to_ps_communication_checksum1 or \
            item == address_of_pl_to_ps_communication_checksum2:
                final_string = final_string + "   | Checksum"
            elif item == address_of_ps_to_pl_communication_indicator1 or \
            item == address_of_ps_to_pl_communication_indicator2 or \
            item == address_of_pl_to_ps_communication_indicator1 or \
            item == address_of_pl_to_ps_communication_indicator2:
                final_string = final_string + "   | Indicator"
            elif item == address_of_ps_to_pl_communication_type_of_packet or \
            item == address_of_pl_to_ps_communication_type_of_packet:
                final_string = final_string + "   | TypeOfThePacket"
            elif item == address_of_ps_to_pl_communication_requested_action_of_the_packet or \
            item == address_of_pl_to_ps_communication_requested_action_of_the_packet:
                final_string = final_string + "   | RequestedActionOfThePacket"
            elif item == address_of_ps_to_pl_communication_start_of_data or \
            item == address_of_pl_to_ps_communication_start_of_data:
                final_string = final_string + "   | Start of Optional Data"

            #
            # Print contents of BRAM
            #
            file.write(final_string + "\n")
            print(final_string)

    print("\n===================================================================\n")


#
# Define a function to extract value of symbol
#
def get_symbol_value(dut, value):
    element_value = getattr(dut.debuggerMainModule.outputPin_scriptExecutionEngineModule, value)
    hex_string_value = ""
    try:
        int_content_value = int(str(element_value.value), 2)
        hex_string_value = f'{int_content_value:x}'
    except:
        hex_string_value = str(element_value.value)

    final_string_value = f'{value}:   0x{hex_string_value}' + " (bin: " + str(element_value.value) + ")"
    return final_string_value

#
# Define a function to extract type of symbol
#
def get_symbol_type(dut, type):
    element_type = getattr(dut.debuggerMainModule.outputPin_scriptExecutionEngineModule, type)
    hex_string_type = ""
    try:
        int_content_type = int(str(element_type.value), 2)
        hex_string_type = f'{int_content_type:x}'
    except:
        hex_string_type = str(element_type.value)

    final_string_type = f'{type} :   0x{hex_string_type}' + " (bin: " + str(element_type.value) + ")"
    return final_string_type

#
# Define a function to extract stage index
#
def get_stage_index(dut, stage_index):
    stage_index_str = "stageRegs_" + str(stage_index) + "_stageIndex"
    element_stage_index = getattr(dut.debuggerMainModule.outputPin_scriptExecutionEngineModule, stage_index_str)
    hex_string_stage_index = ""
    try:
        int_content_stage_index = int(str(element_stage_index.value), 2)
        hex_string_stage_index = f'{int_content_stage_index:x}'
    except:
        hex_string_stage_index = str(element_stage_index.value)

    final_string_stage_index = f'{stage_index}:   0x{hex_string_stage_index}' + " (bin: " + str(element_stage_index.value) + ")"
    return final_string_stage_index

#
# Define a function to extract content of stages
#
def extract_stage_details(dut):

    print("Script Stage Registers Configuration:\n")
    all_elements = dir(dut.debuggerMainModule.outputPin_scriptExecutionEngineModule)

    #
    # Define the pattern to match
    #
    pattern_value = re.compile(r'stageRegs_\d+_stageSymbol_Value')
    pattern_type = re.compile(r'stageRegs_\d+_stageSymbol_Type')
    pattern_stage_index = re.compile(r'stageRegs_\d+_stageIndex')

    #
    # Filter the list using the patterns
    #
    filtered_strings_values = [s for s in all_elements if pattern_value.match(s)]
    filtered_strings_types = [s for s in all_elements if pattern_type.match(s)]
    filtered_strings_stage_index = [s for s in all_elements if pattern_stage_index.match(s)]

    #
    # Sort the lists
    #
    sorted_values = sorted(filtered_strings_values, key=extract_number)
    sorted_types = sorted(filtered_strings_types, key=extract_number)
    sorted_stage_index = sorted(filtered_strings_stage_index, key=extract_number)

    #
    # Print the filtered strings
    #
    # print(sorted_values)
    # print(sorted_types)
    # print(sorted_stage_index)

    for index, element in enumerate(sorted_values):

        try:
            final_string_type = get_symbol_type(dut, sorted_types[index])

            #
            # Print the type
            #
            print(final_string_type)
        except:
            print("This stage does not contain a 'type'")

        try:
            final_string_value = get_symbol_value(dut, sorted_values[index])

            #
            # Print the value
            #
            print(final_string_value)
        except:
            print("Unable to get stage 'value' configuration details")

        try:
            final_string_stage_index = get_stage_index(dut, index)

            #
            # Print the stage index
            #
            print("index:    " + final_string_stage_index)

        except:
            print("index:    " + str(index) +": This stage does not contain a 'stage index'")

        print("\n")


        #
        # Check stage enable bit
        #
        try:
            stage_enabled = "stageRegs_" + str(index) + "_stageEnable"
            is_stage_enabled = getattr(dut.debuggerMainModule.outputPin_scriptExecutionEngineModule, stage_enabled)
            print("\t Stage enabled bit: " + str(is_stage_enabled))
        except:
            print("\t Stage enabled bit: (unavailable)")

        try:
            final_string_value = get_symbol_value(dut, "stageRegs_" + str(index) + "_getOperatorSymbol_0_Value")
            final_string_type = get_symbol_type(dut, "stageRegs_" + str(index) + "_getOperatorSymbol_0_Type")

            print("\t Get (0) | " + final_string_type)
            print("\t Get (0) | " + final_string_value)
        except:
            print("\t stage at:" + str(index) + " does not contain a Get (0) buffer")

        print("\n")

        try:
            final_string_value = get_symbol_value(dut, "stageRegs_" + str(index) + "_getOperatorSymbol_1_Value")
            final_string_type = get_symbol_type(dut, "stageRegs_" + str(index) + "_getOperatorSymbol_1_Type")

            print("\t Get (1) | " + final_string_type)
            print("\t Get (1) | " + final_string_value)
        except:
            print("\t stage at:" + str(index) + " does not contain a Get (1) buffer")

        print("\n")

        try:
            final_string_value = get_symbol_value(dut, "stageRegs_" + str(index) + "_setOperatorSymbol_0_Value")
            final_string_type = get_symbol_type(dut, "stageRegs_" + str(index) + "_setOperatorSymbol_0_Type")

            print("\t Set (0) | " + final_string_type)
            print("\t Set (0) | " + final_string_value)
        except:
            print("\t stage at:" + str(index) + " does not contain a Set (0) buffer")

        print("\n\n")

def set_input_pins(dut):
    dut.io_inputPin_0.value = 0
    dut.io_inputPin_1.value = 0
    dut.io_inputPin_2.value = 1
    dut.io_inputPin_3.value = 1
    dut.io_inputPin_4.value = 1
    dut.io_inputPin_5.value = 1
    dut.io_inputPin_6.value = 1
    dut.io_inputPin_7.value = 1
    dut.io_inputPin_8.value = 1
    dut.io_inputPin_9.value = 1
    dut.io_inputPin_10.value = 1
    dut.io_inputPin_11.value = 1
    dut.io_inputPin_12.value = 1
    dut.io_inputPin_13.value = 1
    dut.io_inputPin_14.value = 1
    dut.io_inputPin_15.value = 1
    dut.io_inputPin_16.value = 1
    dut.io_inputPin_17.value = 1
    dut.io_inputPin_18.value = 1
    dut.io_inputPin_19.value = 1
    dut.io_inputPin_20.value = 1
    dut.io_inputPin_21.value = 1
    dut.io_inputPin_22.value = 1
    dut.io_inputPin_23.value = 1
    dut.io_inputPin_24.value = 1
    dut.io_inputPin_25.value = 1
    dut.io_inputPin_26.value = 1
    dut.io_inputPin_27.value = 1
    dut.io_inputPin_28.value = 1
    dut.io_inputPin_29.value = 1
    dut.io_inputPin_30.value = 1
    dut.io_inputPin_31.value = 1

@cocotb.test()
async def DebuggerModuleTestingBRAM_test(dut):
    """Test hwdbg module (with pre-defined BRAM)"""

    #
    # Assert initial output is unknown
    #
    assert LogicArray(dut.io_outputPin_0.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_1.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_2.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_3.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_4.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_5.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_6.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_7.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_8.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_9.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_10.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_11.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_12.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_13.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_14.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_15.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_16.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_17.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_18.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_19.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_20.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_21.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_22.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_23.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_24.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_25.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_26.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_27.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_28.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_29.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_30.value) == LogicArray("X")
    assert LogicArray(dut.io_outputPin_31.value) == LogicArray("X")

    #
    # Create a 10ns period clock on port clock
    #
    clock = Clock(dut.clock, 10, units="ns")
    
    #
    # Start the clock. Start it low to avoid issues on the first RisingEdge
    #
    cocotb.start_soon(clock.start(start_high=False))
    
    dut._log.info("Initialize and reset module")

    #
    # Initial values
    #
    dut.io_en.value = 0
    dut.io_plInSignal.value = 0

    #
    # Reset DUT
    #
    dut.reset.value = 1
    for _ in range(10):
        await Timer(10, units="ns")
    dut.reset.value = 0


    dut._log.info("Enabling an interrupting chip to receive commands from BRAM")

    #
    # Enable chip
    #
    dut.io_en.value = 1

    #
    # Set initial input value to prevent it from floating
    #
    dut._log.info("Initializing input pins")
    # set_input_pins(dut)

    #
    # Tell the hwdbg to receive BRAM results
    #
    dut.io_plInSignal.value = 1
    await Timer(10, units="ns")
    dut.io_plInSignal.value = 0

    #
    # Synchronize with the clock. This will regisiter the initial `inputPinX` value
    #
    await Timer(10, units="ns")

    #
    # Wait until the debuggee sends an interrupt to debugger
    #
    clock_counter = 0
    interrupt_not_delivered = False

    while str(dut.io_psOutInterrupt) != "1":
        
        # print("State of interrupt: '" + str(dut.io_psOutInterrupt)+ "'")

        if clock_counter % 10 == 0:
            print("Number of clock cycles spent in debuggee (PL): " + str(clock_counter))
        
        clock_counter = clock_counter + 1
        await Timer(10, units="ns")

        #
        # Apply a limitation to the number of clock cycles that
        # can be executed to avoid infinite time
        #
        if (clock_counter >= maximum_number_of_clock_cycles):
            interrupt_not_delivered = True
            break

    #
    # Being here means either the debuggee sent an interrupt to the PS
    # or the maximum clock cycles reached
    #
    if interrupt_not_delivered:
        print("Maximum clock cycles reached")
    else:    
        print("Debuggee (PL) interrupted Debugger (PS)")

    #
    # Run one more clock cycle to apply the latest BRAM modifications
    #
    await Timer(10, units="ns")

    #
    # Print contents of BRAM
    #
    print_bram_content(dut)

    #
    # Print the script stage configuration
    #
    extract_stage_details(dut)
    
    #
    # Check the final input on the next clock and run the circuit for a couple
    # of more clock cycles
    #
    for _ in range(100):
        set_input_pins(dut)
        await Timer(10, units="ns")
