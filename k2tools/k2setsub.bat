@echo off
rem
rem ------------------------------------------------------------
rem This batch file should live at %K2_ROOT%\k2tools
rem If it is somewhere else nothing will work 
rem ------------------------------------------------------------
rem
if /i not "%K2_ROOT%"=="" goto rootOk
set ERRORLEVEL=1
echo.
echo !!! k2setup.bat not used. need to call that to set K2_ROOT
echo.
goto :EOF
:rootOk

set K2_TEMPVAR1=%cd%
:loophere
for /F "tokens=1* delims=\" %%i in ("%K2_TEMPVAR1%") do set K2_TEMPVAR2=%%i/%%j
if "%K2_TEMPVAR2%" == "%K2_TEMPVAR1%/" goto doneNow
set K2_TEMPVAR1=%K2_TEMPVAR2%
goto loophere
:doneNow

set K2_SUBPATH=
if /i "%K2_TEMPVAR1%"=="%K2_ROOT%" goto done

for /f %%i in ("%K2_ROOT%") do set __MATCH=%%~pni
for /f %%i in ("%cd%") do set __LEFT=%%~pni
:again
for /f %%i in ("%__LEFT%") do set __CHUNK=%%~ni
if /I "%K2_SUBPATH%"=="" (
    set K2_SUBPATH=%__CHUNK%
) ELSE (
    set K2_SUBPATH=%__CHUNK%\%K2_SUBPATH%
)
for /f %%i in ("%__LEFT%\..") do set __CHECK=%%~pni
if /I "%__MATCH%"=="%__CHECK%" goto done
if /I "\"=="%__CHECK%" goto dead
set __LEFT=%__CHECK%
goto again
:dead
echo error.
set K2_SUBPATH=
:done
for /f "tokens=1,* delims=\" %%i in ("%K2_SUBPATH%") do set __LEFT=%%i&&set __CHUNK=%%j

if /I not "%__LEFT%"=="src" goto notsrc
set K2_SUBROOT=%__LEFT%
set K2_SUBPATH=%__CHUNK%
goto reallydone
:notsrc

if /I not "%__LEFT%"=="bin" goto notbin
set K2_SUBROOT=%__LEFT%
set K2_SUBPATH=%__CHUNK%
goto reallydone
:notbin

if /I not "%__LEFT%"=="obj" goto notobj
set K2_SUBROOT=%__LEFT%
for /f "tokens=1,* delims=\" %%i in ("%__CHUNK%") do set K2_SUBPATH=%%j
goto reallydone
:notobj

if /I not "%__LEFT%"=="tgt" goto nottgt
set K2_SUBROOT=%__LEFT%
for /f "tokens=1,* delims=\" %%i in ("%__CHUNK%") do set K2_SUBPATH=%%j
goto reallydone
:nottgt

set K2_SUBROOT=

:reallydone

set K2_TEMPVAR1=%K2_SUBPATH%
:loophere2
for /F "tokens=1* delims=\" %%i in ("%K2_TEMPVAR1%") do set K2_TEMPVAR2=%%i/%%j
if "%K2_TEMPVAR2%" == "%K2_TEMPVAR1%/" goto doneNow2
set K2_TEMPVAR1=%K2_TEMPVAR2%
goto loophere2
:doneNow2
set K2_SUBPATH=%K2_TEMPVAR1%

set __MATCH=
set __LEFT=
set __CHUNK=
set __CHECK=
set K2_TEMPVAR1=
set K2_TEMPVAR2=

if /i not "%K2_SUBPATH%"=="" echo %K2_SUBPATH%
