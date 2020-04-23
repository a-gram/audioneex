@echo off
::
::  Audioneex bootstrap build script
::  --------------------------------
::
::  Usage: 
::
::  > .\build [options]
::
::  where [options] is one or more of the following
::
::  TARGET         = windows | android
::  ARCH           = x32 | x64  (Windows)
::                   armeabi-v7a | arm64-v8a | x86 | x86_64 (Android)
::  API            = API version number (Android only)
::  BINARY_TYPE    = dynamic | static
::  BUILD_MODE     = debug | release
::  DATASTORE_T    = TCDataStore | CBDataStore
::  WITH_EXAMPLES  = ON | OFF
::  WITH_TESTS     = ON | OFF  (for project developers only)
::  NO_RTTI        = ON | OFF
::
::  The paramaters are all optional and if none is specified the target and 
::  architecture will be automatically detected from the environment, while
::  the others will be set to default values (dynamic binary, release mode,
::  TCDataStore and no WITH_* options).
::
::

SETLOCAL

set "PARAMS=%*"
set LPARAMS=%PARAMS%
set DPARAMS=
set BUILD_CMD=cmake --build .
set INSTA_CMD=cmake --build . --target install
set TEST_CMD=cmake --build . --target test

:nextp
for /F "tokens=1,* delims= " %%a in ("%LPARAMS%") do (
   set "DPARAMS=%DPARAMS% -DAX_%%a"
   set LPARAMS=%%b
   goto :nextp
)

call :get_param_value "TARGET" TARGET_PLATFORM
if not defined TARGET_PLATFORM set TARGET_PLATFORM=windows
call :bootstrap_%TARGET_PLATFORM%

goto :do_build


::=================
:bootstrap_windows
::=================

echo.
echo -- Building for Windows
echo.
set "MAKE_CMD=cmake -G "NMake Makefiles" %DPARAMS% .."

exit /B 0


::=================
:bootstrap_android
::=================

echo.
echo -- Building for Android
echo.
call :get_ndk_home
if %ERRORLEVEL% EQU 1 goto :eof
call :get_param_value "ARCH" __ARCH
call :get_param_value "API" __API
if not defined __API set __API=latest

set __TOOLCHAIN_FILE=%NDK_HOME%/build/cmake/android.toolchain.cmake
set __CMAKE_PROGRAM=%NDK_HOME%/prebuilt/windows-x86_64/bin/make.exe

set MAKE_CMD=cmake -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="%__TOOLCHAIN_FILE%"
set MAKE_CMD=%MAKE_CMD% -DCMAKE_MAKE_PROGRAM="%__CMAKE_PROGRAM%"
set MAKE_CMD=%MAKE_CMD% -DANDROID_NDK="%NDK_HOME%"
set MAKE_CMD=%MAKE_CMD% -DANDROID_ABI=%__ARCH%
set MAKE_CMD=%MAKE_CMD% -DANDROID_PLATFORM=%__API%
set MAKE_CMD=%MAKE_CMD% -DANDROID_TOOLCHAIN=clang
set MAKE_CMD=%MAKE_CMD% %DPARAMS% ..

exit /B 0


::============
:get_ndk_home
::============

set NDK_HOME=%ANDROID_NDK_HOME%
if not defined NDK_HOME (
   set NDK_HOME=%ANDROID_NDK_ROOT%
   if not defined NDK_HOME (
      echo.
      echo ERROR:
      echo   Could not find the NDK. Please set either the
      echo   ANDROID_NDK_HOME or ANDROID_NDK_ROOT environment
      echo   variable with the path to your NDK installation
      echo   directory.
      echo.
      exit /B 1
   )
)
exit /B 0


::===============
:get_param_value
::===============

set __PARAMS=%PARAMS%
:nextpp
for /F "tokens=1,* delims= " %%a in ("%__PARAMS%") do (
   for /F "tokens=1,2 delims==" %%c in ('echo %%a^|findstr /R "%~1="') do (
      set "%2=%%d"
      goto :eof
   )
   set __PARAMS=%%b
   goto :nextpp
)
exit /B 0


::========
:do_build
::========

if not defined MAKE_CMD exit /B 1
call :get_param_value "WITH_TESTS" __WITH_TESTS
if not defined __WITH_TESTS set TEST_CMD=rem

if exist _build rmdir /s /q _build
mkdir _build && cd _build
%MAKE_CMD% && %BUILD_CMD% && %INSTA_CMD% && %TEST_CMD% && cd ..

