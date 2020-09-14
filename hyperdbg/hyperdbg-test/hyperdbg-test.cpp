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

#include <Windows.h>
#include <iostream>
#include <string.h>

using namespace std;

int main(int argc, char *argv[]) {

  if (argc != 2) {
    printf("you should not test functionalities directly, instead use 'test' "
           "command from HyperDbg...\n");
    return 1;
  }

  if (!strcmp(argv[1], "im-hyperdbg")) {

    //
    // It's not called directly, it's probably from HyperDbg
    //
    printf("hello");

  } else {
    printf("you should not test functionalities directly, instead use 'test' "
           "command from HyperDbg...\n");
    return 1;
  }
  return 0;
}
