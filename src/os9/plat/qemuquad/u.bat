@echo off
if /i "%1"=="" goto :end
call attach c:\repo\k2os\src\uefi\QemuQuadPkg\sdcard.vhd
rd /s /q %1%\efi
rd /s /q %1%\k2os
xcopy /Y c:\repo\k2os\img\a32\debug\os9\plat\qemuquad\bootdisk\*.* %1%\ /e /s
copy /Y c:\repo\k2os\src\uefi\Build\QemuQuad\DEBUG_GCC48\FV\BOARDDXE.Fv %1%\
call detach c:\repo\k2os\src\uefi\QemuQuadPkg\sdcard.vhd
:end