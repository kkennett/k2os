@echo off
if /i "%1"=="" goto :end
call attach c:\repo\k2os\vhd\k2os.vhd
rd /s /q %1%\efi
rd /s /q %1%\k2os
xcopy /Y c:\repo\k2os\img\x32\debug\os9\plat\virtbox\bootdisk\*.* %1%\ /e /s
call detach c:\repo\k2os\vhd\k2os.vhd
:end