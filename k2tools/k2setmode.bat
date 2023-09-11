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
if /I "%1"=="debug" goto setDEBUG
if /I "%1"=="final" goto setFINAL
if /I "%1"=="" goto setNone
set ERRORLEVEL=1
echo.
echo !!! k2setmode needs an mode argument (debug/final)
echo.
goto done
:setDEBUG
set K2_MODE=DEBUG
goto done
:setFINAL
set K2_MODE=FINAL
goto done
:setNone
set K2_MODE=
:done
