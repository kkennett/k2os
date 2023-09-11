@echo off
if /i "%1"=="" goto :end
rd /s /q %1%\efi
rd /s /q %1%\k2os
xcopy /Y c:\repo\k2os\img\a32\debug\os9\plat\udoo\bootdisk\*.* %1%\ /e /s
:end