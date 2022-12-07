@echo off
cd ..
setlocal enableextensions enabledelayedexpansion

set msbuildemitsolution=1


set vswhere="!ProgramFiles(x86)!\Microsoft Visual Studio\Installer\vswhere.exe"
set vcvarsall_path=""

for /f "tokens=*" %%i in ('!vswhere! -property installationPath') do (   
  if exist "%%i\VC\Auxiliary\Build\vcvarsall.bat" (
	  echo.
	  echo vcvarsall.bat found : %%i 
	  echo.

	  set "vcvarsall_path="%%i\VC\Auxiliary\Build\vcvarsall.bat""
  )
)

if !vcvarsall_path! == "" (
    echo.
    echo Error, could not find vcvarsall.bat
    echo.
	
	endlocal
	exit /b 1
)

@rem call MSVC Developer Command Prompt
call !vcvarsall_path! x86_amd64

msbuild.exe hyperdbg\hyperdbg.sln /t:Build /p:Configuration=Debug;Platform=x64 -m

if %errorlevel% == 0 (
    echo.
    echo The solution built successfully!
    echo.
) else (
    echo.
    echo Error 
    echo.
)

endlocal

pause
exit /b 0