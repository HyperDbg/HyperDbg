# HyperDbg for Linux

## Status

⚠️ **HyperDbg for Linux is not yet ready.**

Linux is currently **not supported**.

We are in the process of porting HyperDbg to Linux. This effort is ongoing and may take some time to complete.

# Contributing to the Linux Port

HyperDbg is being ported from Windows to Linux. The work is incremental: most
of the codebase compiles file-by-file as the Win32-specific calls get replaced
with a platform-independent interface.

## Building

```bash
cmake .       # generate the Makefiles
make          # build
```

Run these from the repo root (or the relevant subdirectory). `cmake .` only
needs to be re-run when the CMake files change; otherwise `make` is enough.

## How to contribute

The port progresses one source file at a time. To pick up work:

1. Build and find the next file that fails to compile.
2. Go through it and make it Linux-compatible.
3. Where the code calls Windows-only APIs (serial, registry, signals,
   threads, etc.), don't `#ifdef` inline — route the call through the
   **platform-independent interface** so both Windows and Linux share the same
   call site with separate backing implementations.
4. Rebuild until the file is clean, then move to the next one.

## The platform interface

User-mode abstractions live in `include/platform/user/` (`header/` for the
interface, `code/` for the implementations). See existing examples such as
`platform-serial`, `platform-signal`, and `platform-ioctl` for the pattern to
follow. Kernel-mode equivalents are under `include/platform/kernel/`.

## Guidelines

- Keep platform-specific code behind the platform abstraction, not scattered
  through the logic.
- Match the surrounding code's style and conventions.
- Build before submitting — every changed file should compile.