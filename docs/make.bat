@ECHO OFF

pushd %~dp0

REM Command file for Sphinx documentation

set SOURCEDIR=.
set BUILDDIR=_build
set DOXYGENDIR=_doxygen
set BUILDER=%1

REM Clean up previous builds
for /D %%d in (%BUILDDIR% %DOXYGENDIR%) do ( 
   if exist %%d (
       echo.Removing %%d ...
       rmdir /S /Q %%d
   )
)

if "%SPHINXBUILD%" == "" (
	set SPHINXBUILD=sphinx-build
)

if "%BUILDER%" == "" (
    set BUILDER=html
)

%SPHINXBUILD% >NUL 2>NUL
if errorlevel 9009 (
	echo.
	echo.The 'sphinx-build' command was not found. Make sure you have Sphinx
	echo.installed, then set the SPHINXBUILD environment variable to point
	echo.to the full path of the 'sphinx-build' executable. Alternatively you
	echo.may add the Sphinx directory to PATH.
	echo.
	echo.If you don't have Sphinx installed, grab it from
	echo.http://sphinx-doc.org/
	exit /b 1
)

%SPHINXBUILD% -M %BUILDER% %SOURCEDIR% %BUILDDIR% %SPHINXOPTS% %O%
start "" "%BUILDDIR%/%BUILDER%/index.html"
goto end

:help
%SPHINXBUILD% -M help %SOURCEDIR% %BUILDDIR% %SPHINXOPTS% %O%

:end
popd
