@echo off
setlocal EnableDelayedExpansion

where /q cl || (
  echo MSVC's cl is not found - please run this from the MSVC x64 native tools command prompt
  exit /b 1
)

set code_dir=%~dp0
set build_dir=!code_dir!\build
if not exist !build_dir! mkdir !build_dir!

set common_compile_flags=-W4 -O2
set common_linker_flags=-link -nologo

REM Build the metaprogram
pushd !build_dir!
    cl !common_compiler_flags! !code_dir!\bt_secp256k1_metaprogram.cpp !common_linker_flags! || (
        echo Build script failed because the metaprogram failed to build successfully
        exit
    )
popd

REM Code generate the single file header
!build_dir!\bt_secp256k1_metaprogram.exe !code_dir!\secp256k1 || (
    echo Build script failed because the metaprogram did not run successfully
    exit
)

REM Build the test program to sanity check the single file header
pushd !build_dir!
    set common_source_files=!code_dir!\bt_secp256k1_example.c

    REM Build a C/C++ i64 version
    cl !common_compile_flags! -Tc !common_source_files! !common_linker_flags! -out:bt_secp256k1_i64_example_c.exe
    cl !common_compile_flags! -Tp !common_source_files! !common_linker_flags! -out:bt_secp256k1_i64_example_cpp.exe

    where /q clang-cl || (
      exit
    )

    REM (Optional) Build a C/C++ {i128, i128 x86_64 ASM} version
    REM This requires clang-cl because MSVC does not expose a uint128_t type
    clang-cl !common_compile_flags! -D EXAMPLE_BUILD_I128            -Tc !common_source_files! !common_linker_flags! -out:bt_secp256k1_i128_example_c.exe
    clang-cl !common_compile_flags! -D EXAMPLE_BUILD_I128_X86_64_ASM -Tc !common_source_files! !common_linker_flags! -out:bt_secp256k1_i128_x86_64_asm_example_c.exe

    clang-cl !common_compile_flags! -D EXAMPLE_BUILD_I128            -Tp !common_source_files! !common_linker_flags! -out:bt_secp256k1_i128_example_cpp.exe
    clang-cl !common_compile_flags! -D EXAMPLE_BUILD_I128_X86_64_ASM -Tp !common_source_files! !common_linker_flags! -out:bt_secp256k1_i128_x86_64_asm_example_cpp.exe
popd
