@ECHO OFF

::  This script will build the final distribution libraries (Augh!)
::
::  Usage:  build_dist_libs plat arch comp bmode btype
::
::  where:  plat = win
::          arch = x32|x64
::          comp = vc11|vc12|vc141
::         bmode = debug|release
::         btype = static|dynamic
::
::  NOTE: External dependencies paths (such as fftss) may need to be
::        set up properly according to the user's environment. Please
::        see the 'Library paths' section below.
::

SET ARGSNUM=0
for %%i in (%*) do set /A ARGSNUM+=1

if %ARGSNUM% NEQ 5 (
  SET AX_ERROR=Wrong number of arguments
  goto error
)

SET BUILD_PLAT=
SET BUILD_ARCH=
SET BUILD_COMP=
SET BUILD_MODE=
SET BUILD_BTYPE=

for %%a in (win)              do  if /i "%1"=="%%a" SET BUILD_PLAT=%1
for %%a in (x32 x64)          do  if /i "%2"=="%%a" SET BUILD_ARCH=%2
for %%a in (vc11 vc12 vc141)  do  if /i "%3"=="%%a" SET BUILD_COMP=%3
for %%a in (debug release)    do  if /i "%4"=="%%a" SET BUILD_MODE=%4
for %%a in (static dynamic)   do  if /i "%5"=="%%a" SET BUILD_BTYPE=%5


if "%BUILD_PLAT%"=="" (
  SET AX_ERROR=Invalid platform '%1'
  goto error
)

if "%BUILD_ARCH%"=="" (
  SET AX_ERROR=Invalid architecture '%2'
  goto error
)

if "%BUILD_COMP%"=="" (
  SET AX_ERROR=Invalid compiler '%3'
  goto error
)

if "%BUILD_MODE%"=="" (
  SET AX_ERROR=Invalid build mode '%4'
  goto error
)

if "%BUILD_BTYPE%"=="" (
  SET AX_ERROR=Invalid binary type '%5'
  goto error
)

::::::::::::::::::::::::::::::::::::::::::::::::
::::::::::::::: Build environment ::::::::::::::
::::::::::::::::::::::::::::::::::::::::::::::::

if /i %BUILD_COMP% == vc11  SET VC_SETENV_DIR=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC
if /i %BUILD_COMP% == vc12  SET VC_SETENV_DIR=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC
if /i %BUILD_COMP% == vc141 SET VC_SETENV_DIR=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build

:: Call the build environment setup script. VC uses 'x86' to identify 32-bit architectures
:: and 'x64" for x86/amd 64-bit. We use x32 and x64 for x86 32/64 bit, so let's fix this.

SET VC_ARCH=%BUILD_ARCH%
if "%BUILD_ARCH%" == "x32" SET VC_ARCH=x86
SET VC_SETENV_SCRIPT=vcvarsall.bat

CALL "%VC_SETENV_DIR%\%VC_SETENV_SCRIPT%" %VC_ARCH%

::::::::::::::::::::::::::::::::::::::::::::::::
::::::::::::::: Library paths ::::::::::::::::::
::::::::::::::::::::::::::::::::::::::::::::::::

SET BUILD_LIB_PATH=build\%BUILD_PLAT%-%BUILD_ARCH%-%BUILD_COMP%\%BUILD_MODE%
SET DIST_LIB_PATH=lib\%BUILD_PLAT%-%BUILD_ARCH%-%BUILD_COMP%\%BUILD_MODE%

:: External libraries
:: NOTE: Set these paths according to your system based on build
::       architecture, compiler and mode
SET FFTSS_LIB=%LIBRARIES%\fftss\%BUILD_ARCH%\%BUILD_COMP%\%BUILD_MODE%\fftss.lib

:: Final distribution binaries
SET AUDIONEEX_LIB=%BUILD_LIB_PATH%\audioneex.lib
SET AUDIONEEX_DIST_LIB=%DIST_LIB_PATH%\audioneex.lib

:: Check that the required libraries do exist
if not exist %AUDIONEEX_LIB% (
   SET AX_ERROR=%AUDIONEEX_LIB% does not exist
   goto error
)

if not exist %FFTSS_LIB% (
   SET AX_ERROR=%FFTSS_LIB% does not exist
   goto error
)

if not exist %DIST_LIB_PATH% mkdir %DIST_LIB_PATH%

::::::::::::::::::::::::::::::::::::::::::::::::
::::::::: Distribution libraries build :::::::::
::::::::::::::::::::::::::::::::::::::::::::::::

:: Build distro static libraries if static build, else just copy the build output
if "%BUILD_BTYPE%" == "static" (
   lib /OUT:%AUDIONEEX_DIST_LIB% %AUDIONEEX_LIB% %FFTSS_LIB%
) else (
   copy %BUILD_LIB_PATH%\audioneex.* %DIST_LIB_PATH%
)
goto eof

:::

:error
echo ERROR [build_dist_libs.bat]: %AX_ERROR%
echo %AX_ERROR% > %0_errors.log
exit 1

:eof
exit 0

