/**
 * @file eptcommands.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implement test for EPT related-functions
 * @details
 * @version 0.1
 * @date 2020-07-14
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

BOOLEAN TestMonitorCommand() {

  printf("Test monitor command... \n");

  PVOID Address = malloc(0x1000);

  printf("Allocated Address : %llx\n", Address);

  for (size_t i = 0; i < LONG_MAX; i++) {
    Sleep(1000);
  }

  return FALSE;
}
