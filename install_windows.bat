@ECHO OFF
::
::  This script will install the final binaries into a specific
::  directory (currently a 'lib' directory at the root) and applies
::  some manipulation on the files. It's called by the build 
::  process for Windows builds and there is generally no need to 
::  invoke it explicitly.
::
::  Usage:  install_windows <plat> <arch> <comp> <bmode> <btype>
::
::  where:  <plat> = windows
::          <arch> = x32|x64
::          <comp> = msvc
::         <bmode> = debug|release
::         <btype> = static|dynamic
::


SETLOCAL


::#########################################################
::#                     User Config                      ##
::#########################################################

:: NOTE: These settings are only required for static builds

:: Locations for 32/64 bit release and debug libraries.
set EXT_LIB_x64_release=C:\dev\cpp\lib\vc191\x64\release\static
set EXT_LIB_x64_debug=C:\dev\cpp\lib\vc191\x64\debug\static
set EXT_LIB_x32_release=C:\dev\cpp\lib\vc191\x32\release\static
set EXT_LIB_x32_debug=C:\dev\cpp\lib\vc191\x32\debug\static

:: If your debug libs have a specific suffix, specify it here.
set EXT_LIB_DEBUG_SUFFIX=

:: The default install folder for recent Visual Studio versions.
:: If your VS is installed in a different folder, specify it here.
set VC_INSTALL_DIR=C:\Program Files (x86)\Microsoft Visual Studio

::#########################################################


set ARGSNUM=0

for %%i in (%*) do set /A ARGSNUM+=1

if %ARGSNUM% NEQ 5 (
  set AX_ERROR=Wrong number of arguments
  goto error
)

set BUILD_PLAT=
set BUILD_ARCH=
set BUILD_COMP=%3
set BUILD_MODE=
set BUILD_BTYPE=

for %%a in (windows)        do  if /i "%1"=="%%a" set BUILD_PLAT=%1
for %%a in (x32 x64)        do  if /i "%2"=="%%a" set BUILD_ARCH=%2
:: for %%a in (msvc)           do  if /i "%3"=="%%a" set BUILD_COMP=%3
for %%a in (debug release)  do  if /i "%4"=="%%a" set BUILD_MODE=%4
for %%a in (static dynamic) do  if /i "%5"=="%%a" set BUILD_BTYPE=%5


if "%BUILD_PLAT%"=="" (
  set AX_ERROR=Invalid platform '%1'
  goto error
)

if "%BUILD_ARCH%"=="" (
  set AX_ERROR=Invalid architecture '%2'
  goto error
)

:: if "%BUILD_COMP%"=="" (
::   set AX_ERROR=Invalid compiler '%3'
::   goto error
:: )

if "%BUILD_MODE%"=="" (
  set AX_ERROR=Invalid build mode '%4'
  goto error
)

if "%BUILD_BTYPE%"=="" (
  set AX_ERROR=Invalid binary type '%5'
  goto error
)


if "%BUILD_BTYPE%" == "static" call :static_build
call :install_lib
goto :eof


::===========
:static_build
::===========

set VC_SETENV_SCRIPT=vcvarsall.bat
set VC_SCRIPTS_FOUND=0

echo Looking for the environment setup script. This may take a while ...

:: For static builds we need some VC tools to create the final binary.
:: Actually, we only need the 'lib' tool and could only look for it, but 
:: we would have to determine which one to use based on the architecture, 
:: so we look for vcvarsall.bat to setup the environment for the right tools.
for /f "tokens=*" %%G in ('dir /b /s /a:d "%VC_INSTALL_DIR%"') do (
   if exist %%G\%VC_SETENV_SCRIPT% (
      set VC_SETENV_DIR=%%G
      set /A VC_SCRIPTS_FOUND+=1
   )
)

if %VC_SCRIPTS_FOUND% EQU 1 (
   echo Found %VC_SETENV_SCRIPT% in "%VC_SETENV_DIR%"
)

if %VC_SCRIPTS_FOUND% EQU 0 (
   echo.
   echo   The "%VC_SETENV_SCRIPT%" script was not found under the directory
   echo   "%VC_INSTALL_DIR%".
   echo   If Visual Studio is not installed in that directory then you must 
   echo   specify its location by setting the VC_INSTALL_DIR variable in the 
   echo   User Config section of the install_windows.bat script.
   echo.
   goto error
)

if %VC_SCRIPTS_FOUND% GTR 1 (
   echo.
   echo   Multiple "%VC_SETENV_SCRIPT%" scripts found under the directory
   echo   "%VC_INSTALL_DIR%".
   echo   If there are multiple versions of Visual Studio installed then
   echo   you must specify which one to use by setting the VC_INSTALL_DIR
   echo   variable in the User Config section of install_windows.bat .
   echo.
   goto error
)

:: Call vcvarsall to setup the environment so we can call the right tools.
:: NOTE: VC uses 'x86' for 32-bit architectures and 'x64' for 64-bit. 
:: We use x32 and x64 for x86 32 and 64 bit, so let's fix this first.

set VC_ARCH=%BUILD_ARCH%
if "%BUILD_ARCH%" == "x32" set VC_ARCH=x86

call "%VC_SETENV_DIR%\%VC_SETENV_SCRIPT%" %VC_ARCH%

:: Some debug libraries use suffixes (e.g. _d. d) to distinguish
if /i "%BUILD_MODE%"=="debug" (
   set LIB_SFX=%EXT_LIB_DEBUG_SUFFIX%
) else (
   set LIB_SFX=
)

call set FFTSS_LIB=%%EXT_LIB_%BUILD_ARCH%_%BUILD_MODE%%%\fftss%LIB_SFX%.lib

exit /B 0


::==========
:install_lib
::==========

set LIB_DIR_NAME=%BUILD_PLAT%-%BUILD_ARCH%-%BUILD_COMP%
set BUILD_LIB_DIR=build\%LIB_DIR_NAME%\%BUILD_MODE%
set INSTALL_LIB_DIR=lib\%LIB_DIR_NAME%\%BUILD_MODE%
set AUDIONEEX_LIB=%BUILD_LIB_DIR%\audioneex.lib
set AUDIONEEX_INST_LIB=%INSTALL_LIB_DIR%\audioneex.lib
:: Needs to be defined anyways else will fail
if not defined FFTSS_LIB set FFTSS_LIB=""

:: Check that the required libraries do exist
if not exist %AUDIONEEX_LIB% (
   set AX_ERROR=%AUDIONEEX_LIB% does not exist
   goto error
)

if not exist %INSTALL_LIB_DIR% mkdir %INSTALL_LIB_DIR%

:: For static builds, assemble the library, else just copy the build output
if "%BUILD_BTYPE%" == "static" (
   :: Check whether FFTSS is available
   if not exist %FFTSS_LIB% (
      set AX_ERROR=%FFTSS_LIB% does not exist
      goto error
   )
   lib /OUT:%AUDIONEEX_INST_LIB% %AUDIONEEX_LIB% %FFTSS_LIB%
) else (
   copy %BUILD_LIB_DIR%\audioneex.* %INSTALL_LIB_DIR%
)

exit /B 0


::====
:error
::====

echo.
echo ERROR [%~nx0]: %AX_ERROR%
echo.
echo %AX_ERROR% > %0_errors.log
exit /B 1

