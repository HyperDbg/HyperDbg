/**
 * @file ScriptEngineCommon.h
 * @author M.H. Gholamrezei (gholamrezaei.mh@gmail.com)
 * @brief Shared Headers for Script engine
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

typedef struct SYMBOL {
  long long unsigned Type;
  long long unsigned Value;
} SYMBOL, *PSYMBOL;

typedef struct SYMBOL_BUFFER {
  PSYMBOL Head;
  unsigned int Pointer;
  unsigned int Size;

} SYMBOL_BUFFER, *PSYMBOL_BUFFER;
