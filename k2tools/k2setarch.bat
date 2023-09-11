@echo off
rem
rem ------------------------------------------------------------
rem This batch file should live at %K2_ROOT%\k2tools
rem If it is somewhere else nothing will work 
rem ------------------------------------------------------------
rem
if /i not "%K2_PRE_SETUP_PATH%"=="" goto prePathOk
set ERRORLEVEL=1
echo.
echo !!! k2setup.bat not used. need to call that to set K2_PRE_SETUP_PATH
echo.
goto :EOF
:prePathOk
if /I "%1"=="a32" goto setA32
if /I "%1"=="x32" goto setX32
if /I "%1"=="x64" goto setX64
if /I "%1"=="a64" goto setA64
if /I "%1"=="" goto setNone
set ERRORLEVEL=1
echo.
echo !!! k2setarch needs an arch argument (a32/x32)
echo.
goto done
:setA32
set K2_ARCH=A32
goto done
:setX32
set K2_ARCH=X32
goto done
:setA64
set K2_ARCH=A64
goto done
:setX64
set K2_ARCH=X64
goto done
:setNone
set K2_ARCH=
:done
set K2_TOOLS_BIN_PATH=%K2_ROOT%/k2tools/%K2_ARCH%
set PATH=%K2_PRE_SETUP_PATH%;%K2_ROOT%/k2tools;%K2_TOOLS_BIN_PATH%
