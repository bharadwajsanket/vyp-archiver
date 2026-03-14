# VYP Archive Tool

A lightweight, portable command-line archive utility for creating and managing `.vyp` archives — a custom binary format inspired by tools like `tar`.

---

## Build

```bash
make
```

Produces a single binary: `./vypack`

```bash
make clean   # remove object files and binary
```

**Requirements:** GCC or any C99-compatible compiler, POSIX/Linux/macOS.

---

## Commands

```
vypack init       <archive.vyp>          Create a new empty archive
vypack add        <file> <archive.vyp>   Add a file to an archive
vypack list       <archive.vyp>          List archive contents
vypack extract    <file> <archive.vyp>   Extract a single file
vypack extractall <archive.vyp>          Extract all files
vypack delete     <file> <archive.vyp>   Remove a file from the archive
vypack test       <archive.vyp>          Verify archive integrity
```

---

## Usage Examples

```bash
# Create a new archive
./vypack init myarchive.vyp

# Add files
./vypack add hello.txt   myarchive.vyp
./vypack add data.csv    myarchive.vyp
./vypack add firmware.bin myarchive.vyp

# List contents
./vypack list myarchive.vyp
# Files: 3
#   hello.txt    (128 bytes)
#   data.csv     (4096 bytes)
#   firmware.bin (131072 bytes)

# Extract a single file
./vypack extract hello.txt myarchive.vyp

# Extract everything
./vypack extractall myarchive.vyp

# Delete a file (archive is compacted automatically)
./vypack delete data.csv myarchive.vyp

# Verify archive integrity
./vypack test myarchive.vyp
# Archive OK
```

---

## Archive Format

```
┌──────────────────────────────────────────────┐
│  VypHeader  (16 bytes)                       │
│    magic[4]        "VYP1"                    │
│    version         uint32                    │
│    index_offset    uint64                    │
├──────────────────────────────────────────────┤
│  File data block 0                           │
│  File data block 1                           │
│  ...                                         │
├──────────────────────────────────────────────┤
│  Index                                       │
│    file_count      uint64                    │
│    ┌─ Entry 0 ──────────────────────────┐    │
│    │  path_len   uint16                 │    │
│    │  offset     uint64                 │    │
│    │  size       uint64                 │    │
│    │  path       (path_len bytes)       │    │
│    └────────────────────────────────────┘    │
│    Entry 1 ...                               │
└──────────────────────────────────────────────┘
```

- The header's `index_offset` always points to the start of the index block.
- File data is stored contiguously immediately after the header.
- The index is appended after the last data block.
- On `delete`, the archive is fully compacted into a new file: the deleted entry's data block is omitted and all offsets are recalculated.
- Path strings are stored without a null terminator on disk; the null is added in memory on load.

---

## Project Structure

```
vyp/
├── src/
│   ├── main.c       Entry point
│   ├── cli.c        Command parsing and dispatch
│   ├── archive.c    Archive operations (create, add, extract, delete, test)
│   └── index.c      Index I/O and validation
├── include/
│   ├── vyp_format.h On-disk format structs and constants
│   ├── archive.h    Archive API
│   ├── index.h      Index API
│   └── cli.h        CLI API
├── tests/
│   └── basic_test.sh  Integration test suite (31 assertions)
├── Makefile
└── README.md
```

---

## Running Tests

```bash
make
VYPACK=./vypack bash tests/basic_test.sh
```

The test script creates a temporary working directory, exercises every command including all error paths, verifies binary round-trip integrity with MD5, and cleans up after itself.

---

## Limits

| Limit              | Value      |
|--------------------|------------|
| Max files/archive  | 1,024      |
| Max path length    | 511 chars  |
| I/O buffer size    | 64 KiB     |
