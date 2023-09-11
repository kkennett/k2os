@echo off
if /i "%1"=="" goto :end
call attach c:\repo\k2os\src\uefi\QemuQuadPkg\sdcard.vhd
copy /Y Build\QemuQuad\DEBUG_GCC48\FV\BOARDDXE.Fv %1%\
call detach c:\repo\k2os\src\uefi\QemuQuadPkg\sdcard.vhd
:end