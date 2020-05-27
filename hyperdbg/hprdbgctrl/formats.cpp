/**
 * @file formats.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .formats command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

void CommandFormatsHelp() {
  ShowMessages(".formats : Show a value or register in different formats.\n\n");
  ShowMessages("syntax : \t.formats [hex value | register]\n");
}
void CommandFormats(vector<string> SplittedCommand) {

  UINT64 u64Value;
  time_t t;
  struct tm *tmp;
  char MY_TIME[50];
  char Character;

  if (SplittedCommand.size() != 2) {
    ShowMessages("incorrect use of '.formats'\n\n");
    CommandFormatsHelp();
    return;
  }
  if (!ConvertStringToUInt64(SplittedCommand.at(1), &u64Value)) {
    ShowMessages("incorrect use of '.formats'\n\n");
    CommandFormatsHelp();
    return;
  }

  time(&t);

  //
  // localtime() uses the time pointed by t ,
  // to fill a tm structure with the values that
  // represent the corresponding local time.
  //

  tmp = localtime(&t);

  //
  // using strftime to display time
  //
  strftime(MY_TIME, sizeof(MY_TIME), "%x - %I:%M%p", tmp);

  ShowMessages("Evaluate expression:\n");
  ShowMessages("Hex :        %s\n", SeparateTo64BitValue(u64Value).c_str());
  ShowMessages("Decimal :    %d\n", u64Value);
  ShowMessages("Octal :      %o\n", u64Value);

  ShowMessages("Binary :     ");
  PrintBits(sizeof(UINT64), &u64Value);

  ShowMessages("\nChar :       ");
  //
  // iterate through 8, 8 bits (8*6)
  //
  for (size_t j = 0; j < 8; j++) {

    Character = (char)(((char *)&u64Value)[j]);

    if (isprint(Character)) {
      ShowMessages("%c", Character);
    } else {
      ShowMessages(".");
    }
  }
  ShowMessages("\nTime :       %s\n", MY_TIME);
  ShowMessages("Float :      %4.2f %+.0e %E\n", u64Value, u64Value, u64Value);
  ShowMessages("Double :     %.*e\n", DECIMAL_DIG, u64Value);
}
