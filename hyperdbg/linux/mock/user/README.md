# mock — HyperDbg Hello World

A minimal user-mode Linux application that prints **"Hello world HyperDbg!"** to stdout and imports HyperDbg SDK.

---

---

## Requirements

- GCC (any reasonably recent version)
- GNU Make
- Linux (user-mode, no special privileges needed)

Install on Debian/Ubuntu:

```bash
sudo apt update && sudo apt install build-essential
```

---

## Build

```bash
make
```

This compiles `mock.c` into an executable called `mock`.

---

## Run

```bash
./mock
```

Expected output:

```
Hello world HyperDbg!
```

---

## Clean

Remove compiled objects and the binary:

```bash
make clean
```
