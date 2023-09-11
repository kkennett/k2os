@echo detach "%1"
@echo select vdisk file="%1" > dpartcmd.txt
@echo detach vdisk >> dpartcmd.txt
@diskpart /s %cd%\dpartcmd.txt
@del dpartcmd.txt
