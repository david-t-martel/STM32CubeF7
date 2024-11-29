@echo off

echo Please read ReadMe.txt!
pause

copy Binary\ProductDemo_STM32F746G_DISCO.bin Flash.bin>nul
Jlink.exe -device STM32F746NG -CommanderScript CommandFile.jlink
