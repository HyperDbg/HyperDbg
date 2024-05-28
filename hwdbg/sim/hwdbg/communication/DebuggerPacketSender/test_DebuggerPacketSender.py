##
# @file test_DebuggerPacketSender.py
#
# @author Sina Karvandi (sina@hyperdbg.org)
#
# @brief Testing module for DebuggerPacketSender
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

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import Timer
from cocotb.types import LogicArray

'''
  input         clock,
                reset,
                io_en,
  output        io_psOutInterrupt,
  output [12:0] io_rdWrAddr,
  output        io_wrEna,
  output [31:0] io_wrData,
  input         io_beginSendingBuffer,
                io_noNewDataSender,
                io_dataValidInput,
  output        io_sendWaitForBuffer,
                io_finishedSendingBuffer,
  input  [31:0] io_requestedActionOfThePacketInput,
                io_sendingData
'''

@cocotb.test()
async def DebuggerPacketSender_test(dut):
    """Test DebuggerPacketSender module"""

    #
    # Assert initial output is unknown
    #
    assert LogicArray(dut.io_psOutInterrupt.value) == LogicArray("X")
    assert LogicArray(dut.io_rdWrAddr.value) == LogicArray("XXXXXXXXXXXXX")
    assert LogicArray(dut.io_wrEna.value) == LogicArray("X")
    assert LogicArray(dut.io_wrData.value) == LogicArray("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX")
    assert LogicArray(dut.io_sendWaitForBuffer.value) == LogicArray("X")
    assert LogicArray(dut.io_finishedSendingBuffer.value) == LogicArray("X")

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

        dut._log.info("Enable sending data on the chip (" + str(test_number) + ")")

        #
        # Still there is data to send
        #
        dut.io_noNewDataSender.value = 0

        #
        # Tell the sender to start sending data (This mainly operates based on
        # a rising-edge detector, so we'll need to make it low)
        #
        dut.io_beginSendingBuffer.value = 1
        await Timer(10, units="ns")
        dut.io_beginSendingBuffer.value = 0

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
