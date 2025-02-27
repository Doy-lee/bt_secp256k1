@echo off
setlocal EnableDelayedExpansion

where /q cl || (
  echo MSVC's cl is not found - please run this from the MSVC x64 native tools command prompt
  exit /b 1
)

set code_dir=%~dp0
set build_dir=%code_dir%\build
if not exist %build_dir% mkdir %build_dir%

set compile_flags=-W4 -Z7
set linker_flags=-link -nologo

:: Build the test program to sanity check the single file header
pushd %build_dir%
  set source_files=%code_dir%\bt_secp256k1_example.c

  :: MSVC cl. Note MSVC does not support native int128
  cl %compile_flags% -D USE_ARM_X86_64                                                       -Tc %source_files% %linker_flags% -out:bt_secp256k1_x86_64_asm_example_c_msvc.exe || goto :eof
  cl %compile_flags% -D USE_FORCE_WIDEMUL_INT128_STRUCT                                      -Tc %source_files% %linker_flags% -out:bt_secp256k1_int128_struct_example_c_msvc.exe || goto :eof
  cl %compile_flags% -D USE_FORCE_WIDEMUL_INT64                                              -Tc %source_files% %linker_flags% -out:bt_secp256k1_int64_example_c_msvc.exe || goto :eof

  cl %compile_flags% -D USE_ARM_X86_64                  -D SECP256K1_CPLUSPLUS_TEST_OVERRIDE -Tp %source_files% %linker_flags% -out:bt_secp256k1_x86_64_asm_example_cpp_msvc.exe || goto :eof
  cl %compile_flags% -D USE_FORCE_WIDEMUL_INT128_STRUCT -D SECP256K1_CPLUSPLUS_TEST_OVERRIDE -Tp %source_files% %linker_flags% -out:bt_secp256k1_int128_struct_example_cpp_msvc.exe || goto :eof
  cl %compile_flags% -D USE_FORCE_WIDEMUL_INT64         -D SECP256K1_CPLUSPLUS_TEST_OVERRIDE -Tp %source_files% %linker_flags% -out:bt_secp256k1_int64_example_cpp_msvc.exe || goto :eof

  where /q clang-cl || (
    exit
  )

  :: clang-cl. Note CLANG does not support native int128, secp256k1 detects
  :: clang as Linux and attempts to use int128, hence in the x86 builds we
  :: override with implementing int128 math with structs.
  clang-cl %compile_flags% -D USE_ARM_X86_64                  -D USE_FORCE_WIDEMUL_INT128_STRUCT                                      -Tc %source_files% %linker_flags% -out:bt_secp256k1_x86_64_asm_example_c_clang.exe || goto :eof
  clang-cl %compile_flags% -D USE_FORCE_WIDEMUL_INT128_STRUCT                                                                         -Tc %source_files% %linker_flags% -out:bt_secp256k1_int128_struct_example_c_clang.exe || goto :eof
  clang-cl %compile_flags% -D USE_FORCE_WIDEMUL_INT64                                                                                 -Tc %source_files% %linker_flags% -out:bt_secp256k1_int64_example_c_clang.exe || goto :eof

  clang-cl %compile_flags% -D USE_ARM_X86_64                  -D USE_FORCE_WIDEMUL_INT128_STRUCT -D SECP256K1_CPLUSPLUS_TEST_OVERRIDE -Tp %source_files% %linker_flags% -out:bt_secp256k1_x86_64_asm_example_cpp_clang.exe || goto :eof
  clang-cl %compile_flags% -D USE_FORCE_WIDEMUL_INT128_STRUCT -D SECP256K1_CPLUSPLUS_TEST_OVERRIDE                                    -Tp %source_files% %linker_flags% -out:bt_secp256k1_int128_struct_example_cpp_clang.exe || goto :eof
  clang-cl %compile_flags% -D USE_FORCE_WIDEMUL_INT64         -D SECP256K1_CPLUSPLUS_TEST_OVERRIDE                                    -Tp %source_files% %linker_flags% -out:bt_secp256k1_int64_example_cpp_clang.exe || goto :eof
popd
