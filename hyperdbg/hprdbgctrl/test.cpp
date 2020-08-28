/**
 * @file test.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief test command
 * @details
 * @version 0.1
 * @date 2020-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of test command
 * 
 * @return VOID 
 */
VOID CommandTestHelp() {
  ShowMessages(
      "test : Test essential features of HyperDbg in current machine.\n");
  ShowMessages("syntax : \ttest [test case (hex value)\n");

  ShowMessages("\t\te.g : test\n");
  ShowMessages("\t\te.g : test 0x3\n");
}

/**
 * @brief test command handler
 * 
 * @param SplittedCommand 
 * @return VOID 
 */
VOID CommandTest(vector<string> SplittedCommand) {

  BOOLEAN GetTestCase = FALSE;
  BOOLEAN TestEverything = FALSE;
  BOOLEAN AllChecksPerformed = FALSE;
  BOOLEAN WrongTestCaseSpecified = FALSE;
  UINT64 SpecialTestCase = 0;

  if (SplittedCommand.size() > 2) {
    ShowMessages("incorrect use of 'test'\n\n");
    CommandTestHelp();
    return;
  }

  //
  // Interpret command specific details (if any)
  //
  for (auto Section : SplittedCommand) {
    if (!Section.compare("test")) {
      continue;
    } else if (!GetTestCase) {

      //
      // It's probably a test case number
      //
      if (!ConvertStringToUInt64(Section, &SpecialTestCase)) {

        //
        // Unkonwn parameter
        //
        ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
        CommandIoinHelp();
        return;
      } else {
        GetTestCase = TRUE;
      }
    } else {

      //
      // Unkonwn parameter
      //
      ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
      CommandTestHelp();
      return;
    }
  }

  //
  // Perform the test case
  //
  if (SpecialTestCase == DEBUGGER_TEST_ALL_COMMANDS) {

    //
    // Means to check everything
    //
    TestEverything = TRUE;
  }

  //
  // Means to check just one command
  //
  for (size_t i = 1; i < MAXLONG; i++) {
    if (TestEverything) {
      SpecialTestCase = i;
    }
    ShowMessages("---------------------------------------------------------\n");

    switch (SpecialTestCase) {
    case DEBUGGER_TEST_VMM_MONITOR_COMMAND: {
      ShowMessages("[*] Test '!monitor' command...\n");
      if (TestMonitorCommand()) {
        ShowMessages("\t[+] Successful :)\n");
      } else {
        ShowMessages("\t[-] Unsuccessful :(\n");
      }
      break;
    }
    default: {
      if (TestEverything) {
        AllChecksPerformed = TRUE;
      } else {
        WrongTestCaseSpecified = TRUE;
      }
      break;
    }
    }

    //
    // Check if it was just one check
    //
    if (!TestEverything || AllChecksPerformed) {
      
      //
      // Break from loop
      //
      break;
    }
  }

  //
  // Close the line
  //
  if (!TestEverything && !WrongTestCaseSpecified)
  {
      ShowMessages(
          "---------------------------------------------------------\n");
  }

  //
  // Check if command's test-case number was wrong or not
  //
  if (WrongTestCaseSpecified) {
    ShowMessages("The specific test-case doesn't exist.\n");
    ShowMessages("---------------------------------------------------------\n");
  }
}
