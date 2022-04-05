[![Build status](https://ci.appveyor.com/api/projects/status/8e24lcfhp1ltngfu?svg=true)](https://ci.appveyor.com/project/wbenny/pdbex)

# pdbex

pdbex is a utility for reconstructing structures and unions from the [PDB files][msdn-symbols] into compilable C headers.

### Why?

PDB files, among others, contain information about structures and unions.
These information can be very useful - for instance structures and unions from **ntdll.dll** or **ntoskrnl.exe** can be useful for experimenting with Windows internals.
But information in the PDB files are limited only to the symbol name, member name, its type and offset.
Information about nested anonymous structures and unions are lost.
However, with a bit of work, they can be formed back.

I am not aware of any utility which could make a compilable and offset-accurate C header representation of symbols in the PDB file.
Although there do exist [some][headers-mirt] [public][headers-nirsoft] [servers][headers-moonsoft] which list some of the structures, it is only limited subset of various symbols of files of various Windows versions.
Not to say that many of them are not offset-accurate.
The fact that we have [ReactOS][headers-reactos] and [Volatility][headers-volatility] does not help. They will not provide header file for any given PDB file.

### Usage

```c
> pdbex.exe _SID ntdll.pdb

/*
 * PDB file: ntdll.pdb
 * Image architecture: x86
 *
 * Dumped by pdbex tool v0.1, by wbenny
 */

typedef struct _SID_IDENTIFIER_AUTHORITY
{
  /* 0x0000 */ unsigned char Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;

typedef struct _SID
{
  /* 0x0000 */ unsigned char Revision;
  /* 0x0001 */ unsigned char SubAuthorityCount;
  /* 0x0002 */ struct _SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
  /* 0x0008 */ unsigned long SubAuthority[1];
} SID, *PSID;
```

This command will dump not only specified symbol, but also all symbols referenced by it - and in correct order.
If you insist on dumping only the specified symbol, you can disable this feature by **-j-** option:

```c
> pdbex.exe _SID ntdll.pdb -j- -k-

typedef struct _SID
{
  /* 0x0000 */ unsigned char Revision;
  /* 0x0001 */ unsigned char SubAuthorityCount;
  /* 0x0002 */ struct _SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
  /* 0x0008 */ unsigned long SubAuthority[1];
} SID, *PSID;
```

_(**-k-** switch is responsible for ommiting the header.)_

You can even control if definition of referenced symbols should be inlined by **-e [n|i|a]** option.

* n - will not inline anything (unnamed symbols are created separately and named as _TAG_UNNAMED\_###_
* i - will inline only unnamed structures and union (default behavior)
* a - will inline everything

Example of inlining everything:
```c
> pdbex.exe _SID ntdll.pdb -e a -k-

typedef struct _SID
{
  /* 0x0000 */ unsigned char Revision;
  /* 0x0001 */ unsigned char SubAuthorityCount;
  struct _SID_IDENTIFIER_AUTHORITY
  {
    /* 0x0002 */ unsigned char Value[6];
  } IdentifierAuthority;
  /* 0x0008 */ unsigned long SubAuthority[1];
} SID, *PSID;
```

Example of not inlining anything:
```c
> pdbex.exe _LARGE_INTEGER ntdll.pdb -e n -k-

typedef struct _TAG_UNNAMED_1
{
  /* 0x0000 */ unsigned long LowPart;
  /* 0x0004 */ long HighPart;
} TAG_UNNAMED_1, *PTAG_UNNAMED_1;

typedef union _LARGE_INTEGER
{
  union
  {
    struct
    {
      /* 0x0000 */ unsigned long LowPart;
      /* 0x0004 */ long HighPart;
    };
    /* 0x0000 */ struct _TAG_UNNAMED_1 u;
    /* 0x0000 */ __int64 QuadPart;
  };
} LARGE_INTEGER, *PLARGE_INTEGER;

```

Default behavior:
```c
> pdbex.exe _LARGE_INTEGER ntdll.pdb -e i -k-

typedef union _LARGE_INTEGER
{
  union
  {
    struct
    {
      /* 0x0000 */ unsigned long LowPart;
      /* 0x0004 */ long HighPart;
    };
    struct // _TAG_UNNAMED_1
    {
      /* 0x0000 */ unsigned long LowPart;
      /* 0x0004 */ long HighPart;
    } u;
    /* 0x0000 */ __int64 QuadPart;
  };
} LARGE_INTEGER, *PLARGE_INTEGER;

```

You can also dump all symbols using **"\*"** as the symbol name to dump:

```
> pdbex.exe * ntdll.pdb -o ntdll.h
```

This command will dump all structures and unions to the file **ntdll.h**.


### Remarks

* Pointers to functions are represented only as **void\*** with additional comment **/\* function \*/**.
* Produced structures expect **packing alignment to be set at 1 byte**.
* Produced **union**s have one extra **union** nested inside of it (you could notice few lines above). This is a known cosmetic bug.
* **pdbex** is designed to dump headers from C project only - C++ classes are not supported.

### Compilation

Compile **pdbex** using Visual Studio 2017. Solution file is included. No other dependencies are required.

### Testing

There are 2 files in the _Scripts_ folder:

* env.bat - sets environment variables for Microsoft Visual C++ 2015
* test.py - testing script

**test.py** dumps all symbols from the provided PDB file. It also generates C file which tests if offsets of the members of structures and unions do match the original offsets in the PDB file. The C file is then compiled using **msbuild** and ran. If the resulting program prints a line starting with **[!]**, it is considered as error. In that case, line also contains information about struct/union + member + offset that did not match. It prints nothing on success.

Because the **test.py** uses **msbuild** for creating tests, special environment variables must be set. It can be accomplished either by running **test.py** from the developer console or by calling **env.bat**. **env.bat** file exists only for convenience and does nothing else than running the **VsDevCmd.bat** from the default Visual Studio 2015 installation directory. The environment variables are set in the current console process, therefore this script can be called only once.

### Documentation

**pdbex -h** should make it:

```
Version v0.18

pdbex <symbol> <path> [-o <filename>] [-t <filename>] [-e <type>]
                     [-u <prefix>] [-s prefix] [-r prefix] [-g suffix]
                     [-p] [-x] [-m] [-b] [-d] [-i] [-l]

<symbol>             Symbol name to extract
                     Use '*' if all symbols should be extracted.
                     Use '%' if all symbols should be extracted separately.
<path>               Path to the PDB file.
 -o filename         Specifies the output file.                       (stdout)
 -t filename         Specifies the output test file.                  (off)
 -e [n,i,a]          Specifies expansion of nested structures/unions. (i)
                       n = none            Only top-most type is printed.
                       i = inline unnamed  Unnamed types are nested.
                       a = inline all      All types are nested.
 -u prefix           Unnamed union prefix  (in combination with -d).
 -s prefix           Unnamed struct prefix (in combination with -d).
 -r prefix           Prefix for all symbols.
 -g suffix           Suffix for all symbols.

Following options can be explicitly turned off by adding trailing '-'.
Example: -p-
 -p                  Create padding members.                          (T)
 -x                  Show offsets.                                    (T)
 -m                  Create Microsoft typedefs.                       (T)
 -b                  Allow bitfields in union.                        (F)
 -d                  Allow unnamed data types.                        (T)
 -i                  Use types from stdint.h instead of native types. (F)
 -j                  Print definitions of referenced types.           (T)
 -k                  Print header.                                    (T)
 -n                  Print declarations.                              (T)
 -l                  Print definitions.                               (T)
 -f                  Print functions.                                 (F)
 -z                  Print #pragma pack directives.                   (T)
 -y                  Sort declarations and definitions.               (F)
```


### License

All the code in this repository is open-source under the MIT license. See the **LICENSE.txt** file in this repository.

If you find this project interesting, you can buy me a coffee

```
  BTC 3GwZMNGvLCZMi7mjL8K6iyj6qGbhkVMNMF
  LTC MQn5YC7bZd4KSsaj8snSg4TetmdKDkeCYk
```

  [msdn-symbols]: <https://msdn.microsoft.com/en-us/library/windows/desktop/ee416588(v=vs.85).aspx>
  [headers-nirsoft]: <http://www.nirsoft.net/kernel_struct/vista/index.html>
  [headers-moonsoft]: <http://msdn.moonsols.com/>
  [headers-reactos]: <http://doxygen.reactos.org/df/d7e/structETHREAD-members.html>
  [headers-mirt]: <http://msdn.mirt.net/>
  [headers-volatility]: <http://volatilityfoundation.github.io/volatility/classvolatility_1_1plugins_1_1overlays_1_1windows_1_1vista_1_1___e_t_h_r_e_a_d.html>

