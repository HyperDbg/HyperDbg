/**
 * @file hyperdbg-test.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief perform tests on a remote process (this is the remote process)
 * @details
 * @version 0.1
 * @date 2020-09-14
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "NamedPipe.h"
#include <Windows.h>
#include <iostream>
#include <string.h>

using namespace std;

int main(int argc, char *argv[]) {

  HANDLE PipeHandle;
  BOOLEAN SentMessageResult;
  UINT32 ReadBytes;
  const int BufferSize = 1024;
  char Buffer[BufferSize] = "test message to send from client !!!";

  if (argc != 2) {
    printf("you should not test functionalities directly, instead use 'test' "
           "command from HyperDbg...\n");
    return 1;
  }

  if (!strcmp(argv[1], "im-hyperdbg")) {

    ////////////////////////////////////////////////////

    //
    // It's not called directly, it's probably from HyperDbg
    //
    Sleep(1000);


    PipeHandle = NamedPipeClientCreatePipe("\\\\.\\Pipe\\HyperDbgTests");

    if (!PipeHandle) {

      //
      // Unable to create handle
      //
      return 1;
    }

    SentMessageResult =
        NamedPipeClientSendMessage(PipeHandle, Buffer, strlen(Buffer) + 1);

    if (!SentMessageResult) {

      //
      // Sending error
      //
      return 1;
    }

    ReadBytes = NamedPipeClientReadMessage(PipeHandle, Buffer, BufferSize);

    if (!ReadBytes) {

      //
      // Nothing to read
      //
      return 1;
    }

    printf("Server sent the following message: %s\n", Buffer);

    NamedPipeClientClosePipe(PipeHandle);

    ////////////////////////////////////////////////////

  } else {
    printf("you should not test functionalities directly, instead use 'test' "
           "command from HyperDbg...\n");
    return 1;
  }
  return 0;
}
