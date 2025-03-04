# bt\_secp256k1 (Blockchain Tools - secp256k1)

A code-generated single file header for
[bitcoin-core/secp256k1](https://github.com/bitcoin-core/secp256k1), an
optimized C library for ECDSA signatures and secret/public key operations on
curve secp256k1.

## Using bt\_secp256k1

The header files are generated from a metaprogram that is included in this
repository. To use the library you only need to include the header file
`bt_secp256k1.h`.

In exactly one `.c` or `.cpp` file that includes `bt_secp256k1.h` define
`BT_SECP256K1_IMPLEMENTATION` as per follows (with additional options to
configure the library via macros, see the single file headers for more
definitive explanations).

```cpp
#define BT_SECP256K1_IMPLEMENTATION
#define ECMULT_GEN_PREC_BITS 4
#define ECMULT_WINDOW_SIZE 15
#include "bt_secp256k1.h"
```

## Details

The metaprogram provided in `bt_secp256k1_metaprogam.cpp` is responsible for
collating code files and patching source files to ensure the project can be
combined into a single header file, generating the output files.

The code generated header files vendored in the repository correspond to the
current commit hash checked out from the secp256k1 submodule.

An example program `bt_secp256k1_example.c` is provided that implements a basic
secp256k1 keypair generator for reference and testing the single header file
compilation.

## Build

The build is currently tested on Windows Visual Studio 2019's `cl` via
`build.bat` which will build the metaprogram and generate the single header
files.

Examples are built using `build_examples.bat` and optionally invoke `clang-cl`
if it's available on the path.

## License

The single file header code-generator metaprogram is provided under the
Unlicense license.

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

bitcoin-core/secp256k1 is provided under its original license and is embedded in
the single-file-header.
