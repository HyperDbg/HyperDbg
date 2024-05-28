##
# @file test_SendReceiveSynchronizer.py
#
# @author Sina Karvandi (sina@hyperdbg.org)
#
# @brief Testing module for SendReceiveSynchronizer
#
# @details
#
# @version 0.1
#
# @date 2024-04-23
#
# @copyright This project is released under the GNU Public License v3.
#

import random

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import Timer
from cocotb.types import LogicArray

'''
  input         clock,
                reset,
                io_en,
                io_plInSignal,
  output        io_psOutInterrupt,
  output [12:0] io_rdWrAddr,
  input  [31:0] io_rdData,
  output        io_wrEna,
  output [31:0] io_wrData,
                io_requestedActionOfThePacketOutput,
  output        io_requestedActionOfThePacketOutputValid,
  input         io_noNewDataReceiver,
                io_readNextData,
  output        io_dataValidOutput,
  output [31:0] io_receivingData,
  input         io_beginSendingBuffer,
                io_noNewDataSender,
                io_dataValidInput,
  output        io_sendWaitForBuffer,
  input  [31:0] io_requestedActionOfThePacketInput,
                io_sendingData
'''

@cocotb.test()
async def SendReceiveSynchronizer_test(dut):
    """Test SendReceiveSynchronizer module"""

    global DEFAULT_CONFIGURATION_INITIALIZED_MEMORY_SIZE

    #
    # Assert initial output is unknown
    #
    assert LogicArray(dut.io_psOutInterrupt.value) == LogicArray("X")
    assert LogicArray(dut.io_rdWrAddr.value) == LogicArray("XXXXXXXXXXXXX")
    assert LogicArray(dut.io_wrEna.value) == LogicArray("X")
    assert LogicArray(dut.io_wrData.value) == LogicArray("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX")
    assert LogicArray(dut.io_requestedActionOfThePacketOutput.value) == LogicArray("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX")
    assert LogicArray(dut.io_requestedActionOfThePacketOutputValid.value) == LogicArray("X")
    assert LogicArray(dut.io_dataValidOutput.value) == LogicArray("X")
    assert LogicArray(dut.io_receivingData.value) == LogicArray("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX")
    assert LogicArray(dut.io_sendWaitForBuffer.value) == LogicArray("X")

    clock = Clock(dut.clock, 10, units="ns")  # Create a 10ns period clock on port clock
    
    #
    # Start the clock. Start it low to avoid issues on the first RisingEdge
    #
    cocotb.start_soon(clock.start(start_high=False))
    
    dut._log.info("Initialize and reset module")

    #
    # Initial values
    #
    dut.io_en.value = 0
    dut.io_beginSendingBuffer.value = 0
    dut.io_noNewDataSender.value = 0
    dut.io_dataValidInput.value = 0
    dut.io_readNextData.value = 0
    dut.io_noNewDataReceiver.value = 0
    dut.io_plInSignal.value = 0
    dut.io_requestedActionOfThePacketInput.value = 0
    dut.io_requestedActionOfThePacketInput.value = 0

    #
    # Reset DUT
    #
    dut.reset.value = 1
    for _ in range(10):
        await Timer(10, units="ns")
    dut.reset.value = 0

    dut._log.info("Enabling chip")

    #
    # Enable chip
    #
    dut.io_en.value = 1

    for test_number in range(10):

        ###############################################################
        #                                                             #
        #                      Receiving Logic                        #
        #                                                             #
        ###############################################################

        dut._log.info("Enable receiving data on the chip (" + str(test_number) + ")")

        #
        # Tell the receiver to start receiving data (This mainly operates based on
        # a rising-edge detector, so we'll need to make it low)
        #
        dut.io_plInSignal.value = 1
        await Timer(10, units="ns")
        dut.io_plInSignal.value = 0

        #
        # Activate sending logic to test whether the chip fails synchronizing signals or not
        # 
        dut.io_beginSendingBuffer.value = 1
        await Timer(10, units="ns")
        dut.io_beginSendingBuffer.value = 0

        #
        # Wait until the receive operation is done (finished)
        #
        for i in range(30):

            match dut.io_rdWrAddr.value:

                #
                # For the PS data range
                #
                case 0x0: # checksum
                    dut.io_rdData.value = 0x00001234
                case 0x8: # indicator
                    dut.io_rdData.value = 0x48595045 # first 32 bits of the indicator
                case 0x10: # type
                    dut.io_rdData.value = 0x4 # debugger to hardware packet (DEBUGGER_TO_DEBUGGEE_HARDWARE_LEVEL)
                case 0x14: # requested action
                    dut.io_rdData.value = 0x14141414
                case 0x18: # General output
                    dut.io_rdData.value = 0x18181818
                case 0x1c: # General output
                    dut.io_rdData.value = 0x1c1c1c1c
                case 0x20: # General output
                    dut.io_rdData.value = 0x20202020
                case 0x24: # General output
                    dut.io_rdData.value = 0x24242424
                case 0x28: # General output
                    dut.io_rdData.value = 0x28282828
                case 0x2c: # General output
                    dut.io_rdData.value = 0x2c2c2c2c
                case 0x30: # General output
                    dut.io_rdData.value = 0x30303030
                case 0x34: # General output
                    dut.io_rdData.value = 0x34343434
                case 0x38: # General output
                    dut.io_rdData.value = 0x38383838
                case 0x3c: # General output
                    dut.io_rdData.value = 0x3c3c3c3c
                case 0x40: # General output
                    dut.io_rdData.value = 0x40404040
                case 0x44: # General output
                    dut.io_rdData.value = 0x44444444
                case 0x48: # General output
                    dut.io_rdData.value = 0x48484848
                case 0x4c: # General output
                    dut.io_rdData.value = 0x4c4c4c4c
                case 0x50: # General output
                    dut.io_rdData.value = 0x50505050

                #
                # For the PL data range
                #
                case 0x1000: # checksum
                    dut.io_rdData.value = 0x10001000
                case 0x1008: # indicator 1
                    dut.io_rdData.value = 0x48595045 # The first 32 bits of the indicator
                case 0x100c: # indicator 2
                    dut.io_rdData.value = 0x100c100c # The second 32 bits of the indicator
                case 0x1010: # type
                    dut.io_rdData.value = 0x4 # debugger to hardware packet (DEBUGGER_TO_DEBUGGEE_HARDWARE_LEVEL)
                case 0x1014: # requested action
                    dut.io_rdData.value = 0x10141014
                case 0x1018: # General output
                    dut.io_rdData.value = 0x10181018
                case 0x101c: # General output
                    dut.io_rdData.value = 0x101c101c
                case 0x1020: # General output
                    dut.io_rdData.value = 0x10201020
                case 0x1024: # General output
                    dut.io_rdData.value = 0x10241024
                case 0x1028: # General output
                    dut.io_rdData.value = 0x10281028
                case 0x102c: # General output
                    dut.io_rdData.value = 0x102c102c
                case 0x1030: # General output
                    dut.io_rdData.value = 0x10301030
                case 0x1034: # General output
                    dut.io_rdData.value = 0x10341034
                case 0x1038: # General output
                    dut.io_rdData.value = 0x10381038
                case 0x103c: # General output
                    dut.io_rdData.value = 0x103c103c
                case 0x1040: # General output
                    dut.io_rdData.value = 0x10401040
                case 0x1044: # General output
                    dut.io_rdData.value = 0x10441044
                case 0x1048: # General output
                    dut.io_rdData.value = 0x10481048
                case 0x104c: # General output
                    dut.io_rdData.value = 0x104c104c
                case 0x1050: # General output
                    dut.io_rdData.value = 0x10501050
                case _:
                    assert 1 == 2, "invalid address in the address line"

            if dut.io_requestedActionOfThePacketOutputValid.value == 1:

                #
                # No new data needed to be received
                #
                if test_number % 3 == 0:
                    dut.io_noNewDataReceiver.value = 1
                    await Timer(10, units="ns")
                    dut.io_noNewDataReceiver.value = 0
                else:
                    #
                    # Make change to the io_readNextData signal as it operates mainly 
                    # based on a rising-edge detector
                    #
                    if dut.io_readNextData.value == 0:
                        dut.io_readNextData.value = 1
                    else:
                        dut.io_readNextData.value = 0

            #
            # Go to the next clock cycle
            #
            await Timer(10, units="ns")


        if test_number % 3 != 0:
            dut.io_noNewDataReceiver.value = 1
            await Timer(10, units="ns")
            dut.io_noNewDataReceiver.value = 0

        #
        # Run extra waiting clocks
        #
        for _ in range(10):
            await Timer(10, units="ns")

        ###############################################################
        #                                                             #
        #                       Sending Logic                         #
        #                                                             #
        ###############################################################

        dut._log.info("Enable sending data on the chip (" + str(test_number) + ")")

        #
        # There is data to send
        #
        dut.io_noNewDataSender.value = 0
        dut.io_beginSendingBuffer.value = 0

        #
        # Tell the sender to start sending data (This mainly operates based on
        # a rising-edge detector, so we'll need to make it low)
        #
        dut.io_beginSendingBuffer.value = 1
        await Timer(10, units="ns")
        dut.io_beginSendingBuffer.value = 0

        #
        # Activate receiving logic to test whether the chip fails synchronizing signals or not
        # 
        dut.io_plInSignal.value = 1
        await Timer(10, units="ns")
        dut.io_plInSignal.value = 0

        #
        # No new data at this stage
        #
        dut.io_dataValidInput.value = 0
        dut.io_sendingData.value = 0

        #
        # Adjust the requested action of the packet
        #
        dut.io_requestedActionOfThePacketInput.value = 0x55859555

        #
        # Synchronize with the clock. This will apply the initial values
        #
        await Timer(10, units="ns")
        
        #
        # This will change the behavior of the data producer to only
        # generate extra data for 2 of the test case rounds, the third
        # test case doesn't have any extra data
        #
        if test_number % 3 != 0 :
            #
            # Run until the module asks for further buffers
            #
            for i in range(100):
                if dut.io_sendWaitForBuffer.value == 1:
                    val = random.randint(0, 0xffffffff)

                    #
                    # Indicate that the data is valid
                    #
                    dut.io_dataValidInput.value = 1

                    #
                    # Assign the random value to send as the data
                    #
                    dut.io_sendingData.value = val

                await Timer(10, units="ns")

        #
        # Now, tell the sender module that there is no longer needed to send data
        #
        for i in range(100):
            if dut.io_sendWaitForBuffer.value == 1:
                dut.io_noNewDataSender.value = 1
                await Timer(10, units="ns")
                dut.io_noNewDataSender.value = 0
                break
            
            await Timer(10, units="ns")


        #
        # Run extra waiting clocks
        #
        for _ in range(10):
            await Timer(10, units="ns")

        #
        # Check the final input on the next clock
        #
        await Timer(10, units="ns")
