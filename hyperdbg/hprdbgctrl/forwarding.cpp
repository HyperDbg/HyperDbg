/**
 * @file forwarding.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Event source forwarding
 * @details
 * @version 0.1
 * @date 2020-11-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern UINT64 g_OutputSourceTag;

/**
 * @brief Get the output source tag and increase the
 * global variable for tag
 *
 * @return UINT64
 */
UINT64 ForwardingGetNewOutputSourceTag() { return g_OutputSourceTag++; }

/**
 * @brief Opens the output source
 * @param SourceDescriptor Descriptor of the source
 *
 * @return DEBUGGER_OUTPUT_SOURCE_STATUS return status of the opening function
 */
DEBUGGER_OUTPUT_SOURCE_STATUS
ForwardingOpenOutputSource(PDEBUGGER_EVENT_FORWARDING SourceDescriptor) {

  //
  // Check if already closed
  //
  if (SourceDescriptor->State == EVENT_FORWARDING_CLOSED) {
    return DEBUGGER_OUTPUT_SOURCE_STATUS_ALREADY_CLOSED;
  }

  //
  // check if already opened
  //
  if (SourceDescriptor->State == EVENT_FORWARDING_STATE_OPENNED) {
    return DEBUGGER_OUTPUT_SOURCE_STATUS_ALREADY_OPENED;
  }

  //
  // Now, it's time to open the source based on its type
  //

  return DEBUGGER_OUTPUT_SOURCE_STATUS_UNKNOWN_ERROR;
}

/**
 * @brief Closes the output source
 * @param SourceDescriptor Descriptor of the source
 *
 * @return DEBUGGER_OUTPUT_SOURCE_STATUS return status of the closing function
 */
DEBUGGER_OUTPUT_SOURCE_STATUS
ForwardingCloseOutputSource(PDEBUGGER_EVENT_FORWARDING SourceDescriptor) {

  //
  // Check if already closed
  //
  if (SourceDescriptor->State == EVENT_FORWARDING_CLOSED) {
    return DEBUGGER_OUTPUT_SOURCE_STATUS_ALREADY_CLOSED;
  }

  //
  // Check if not opened
  //
  if (SourceDescriptor->State == EVENT_FORWARDING_STATE_NOT_OPENNED ||
      SourceDescriptor->State != EVENT_FORWARDING_STATE_OPENNED) {
    //
    // Not opennd ? or state other than opened ?
    //
    return DEBUGGER_OUTPUT_SOURCE_STATUS_UNKNOWN_ERROR;
  }

  //
  // Now, it's time to close the source based on its type
  //
  if (SourceDescriptor->Type == EVENT_FORWARDING_FILE) {

    //
    // Set the state
    //
    SourceDescriptor->State = EVENT_FORWARDING_CLOSED;

    //
    // Close the hanlde
    //
    CloseHandle(SourceDescriptor->Handle);

    //
    // Return the status
    //
    return DEBUGGER_OUTPUT_SOURCE_STATUS_SUCCESSFULLY_CLOSED;

  } else if (SourceDescriptor->Type == EVENT_FORWARDING_TCP) {
  } else if (SourceDescriptor->Type == EVENT_FORWARDING_NAMEDPIPE) {
  }

  return DEBUGGER_OUTPUT_SOURCE_STATUS_UNKNOWN_ERROR;
}

/**
 * @brief Create a new source (create handle from the source)
 * @param SourceType Type of the source
 * @param Description Description of the source
 *
 * @return DEBUGGER_OUTPUT_SOURCE_STATUS return status of the closing function
 */
HANDLE
ForwardingCreateOutputSource(DEBUGGER_EVENT_FORWARDING_TYPE SourceType,
                             string Description) {

  if (SourceType == EVENT_FORWARDING_FILE) {

    //
    // Create a new file
    //
    HANDLE FileHandle = CreateFileA(Description.c_str(), GENERIC_WRITE, 0, NULL,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    //
    // The handle might be INVALID_HANDLE_VALUE which will be
    // checked by the caller
    //
    return FileHandle;

  } else if (SourceType == EVENT_FORWARDING_NAMEDPIPE) {

  } else if (SourceType == EVENT_FORWARDING_TCP) {
  }

  return INVALID_HANDLE_VALUE;
}
