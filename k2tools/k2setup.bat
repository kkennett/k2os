@echo off
rem
rem ------------------------------------------------------------
rem This batch file should live at %K2_ROOT%\k2tools
rem If it is somewhere else nothing will work 
rem ------------------------------------------------------------
rem
if /I "%K2_ROOT%"=="" goto runIt
echo Setup batch file already run:
set K2
goto :EOF
:runIt
echo ----------------
echo. k2setup.bat
echo ----------------
set ERRORLEVEL=
set K2_TEMPVAR0="%0"
for /f %%I in (%K2_TEMPVAR0%) do set K2_TEMPVAR1=%%~dpI
set K2_TEMPVAR0=
for /f %%I in ("%K2_TEMPVAR1%.") do set K2_TEMPVAR2=%%~nxI
if /I "%K2_TEMPVAR2%"=="k2tools" goto toolsOk
set ERRORLEVEL=1
echo.
echo !!! k2setup.bat file should be in .../k2tools
echo !!! It won't work if it is put somewhere else
echo.
goto :EOF
:toolsOk
for /f %%I in ("%K2_TEMPVAR1%.") do set K2_ROOT=%%~dpI
for /f %%I in ("%K2_ROOT%.") do set K2_ROOT=%%~fI
set K2_ROOT_DOS=%K2_ROOT%
rem
rem
set K2_TEMPVAR1=%K2_ROOT%
:loophere
for /F "tokens=1* delims=\" %%i in ("%K2_TEMPVAR1%") do set K2_TEMPVAR2=%%i/%%j
if "%K2_TEMPVAR2%" == "%K2_TEMPVAR1%/" goto doneNow
set K2_TEMPVAR1=%K2_TEMPVAR2%
goto loophere
:doneNow
set K2_ROOT=%K2_TEMPVAR1%
echo Using K2_ROOT = "%K2_ROOT%"
for /F %%I in ("%K2_ROOT%") do set K2_ROOT_DRIVE=%%~dI
rem
rem
set K2_TEMPVAR2=
set K2_TEMPVAR1=
rem
rem ---------------------------------------
rem K2_ROOT set successfully at this point
rem add tools directory to path
rem ---------------------------------------
rem
pushd "%K2_ROOT_DOS%"
set K2_PRE_SETUP_PATH=%path%
if "%K2_ARCH%"=="" call k2tools/k2setarch x32
echo Using K2_ARCH = "%K2_ARCH%"
if "%K2_MODE%"=="" call k2tools/k2setmode debug
echo Using K2_MODE = "%K2_MODE%"
echo.
call popd
rem ---------------------------------------
rem
