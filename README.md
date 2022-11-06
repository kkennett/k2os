# k2os
k2os source code

All code here is covered by BSD 3-Clause

#How do I build this thing?
- you need Visual Studio 2022 to build the autobuilder.
- in VS2022, open up the solution src\msvc\msvs.sln
- select debug x86 as the build type
- do "rebuild all"
- select 'k2auto' as the startup project
- run (f5).  this will use the autobuilder to build and os images found in the source tree

#How do i run this thing once it's built?
- you need VirtualBox to run the os
- create a virtual machine that uses ICH9 as the controller.  the os can handle up to 8 cpus.
- on the vm, set COM1 to use a host pipe, \\.\pipe\k2osdev 
- on the vm, set it to use the VHD at \vhd\k2os.vhd as its only storage device
- on the vm, set it to boot from hard disk first.
- in windows disk management, attach the VHD at vhd\k2os.vhd.  make sure it has a drive letter
- once you've seen what drive letter the vhd got, detach it.
- in an ADMIN command prompt, change directory to 'src\os8\plat\virtbox'
- use the command 'u <drive letter>:'.  this runs a batch file
- the os on the vhd will be updated to what you just built.
- in the command prompt, run 'dcon.bat'  this will start the debug console
- in virtualbox, start the vm.
