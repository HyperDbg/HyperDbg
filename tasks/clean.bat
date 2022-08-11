@echo off
cd ..
echo Cleaning files ...
echo.

del    *.VC.db  /s
del    *.bsc    /s
:: del    *.pdb    /s   
del    *.iobj   /s
del    *.ipdb   /s
del    *.ilk    /s
del    *.ipch   /s
del    *.obj    /s
del    *.sbr    /s
del    *.tlog   /s
del    *.pch    /s

echo Removing build folders ...
echo.
for /d /r . %%d in (__history Debug Release ipch obj bin) do @if exist "%%d" echo "%%d" && rd /s/q "%%d"

echo Done!
pause
