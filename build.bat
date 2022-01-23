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
    cl !common_compile_flags! !code_dir!\bt_secp256k1_metaprogram.cpp !common_linker_flags! || (
        echo Build script failed because the metaprogram failed to build successfully
        exit
    )
popd

REM Code generate the single file header
!build_dir!\bt_secp256k1_metaprogram.exe !code_dir!\secp256k1 || (
    echo Build script failed because the metaprogram did not run successfully
    exit
)
