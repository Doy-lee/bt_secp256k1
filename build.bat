@echo off
setlocal EnableDelayedExpansion

where /q cl || (
  echo ERROR: cl not found on PATH, ensure it's on the path by calling 'vcvarsall.bat x64' or launching the Visual Studio x64 development terminal
  goto :eof
)

set script_dir_backslash=%~dp0
set script_dir=%script_dir_backslash:~0,-1%
set build_dir=%script_dir%\Build
if not exist %build_dir% mkdir %build_dir%

:: Build the metaprogram
pushd %build_dir%
cl -W4 -Z7 -nologo %script_dir%\bt_secp256k1_metaprogram.cpp -link || (
    echo Build script failed because the metaprogram failed to build successfully
    goto :eof
)
popd

:: Code generate the single file header
%build_dir%\bt_secp256k1_metaprogram.exe %script_dir%\secp256k1 || (
    echo Build script failed because the metaprogram did not run successfully
    goto :eof
)
