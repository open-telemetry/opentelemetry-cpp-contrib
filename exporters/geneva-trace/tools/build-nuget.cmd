Rem Copyright The OpenTelemetry Authors
Rem SPDX-License-Identifier: Apache-2.0

@echo off
pushd "%~dp0"
set "PATH=%CD%;%PATH%"

if not defined PackageVersion (
  echo "Set the PackageVersion before proceeding."
  exit
)
if not exist "..\packages" mkdir "..\packages"

REM This script assumes that the main OpenTelemetry C++ repo
REM has been checked out as submodule
powershell -File .\build-nuget.ps1 %CD%\..\third_party\opentelemetry-cpp

REM If %OUTPUTROOT% variable is defined, then copy the packages
REM to %OUTPUTROOT%\packages for subsequent deployment.
if defined OUTPUTROOT (
  if not exist "%OUTPUTROOT%\packages" mkdir "%OUTPUTROOT%\packages"
  copy /Y ..\packages\*.nupkg "%OUTPUTROOT%\packages"
)
popd
