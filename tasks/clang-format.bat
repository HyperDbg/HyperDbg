@echo off

cd ..

set vswhere="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set clang_format=

cls 
 
setlocal enableextensions enabledelayedexpansion

for /f "tokens=*" %%i in ('%vswhere% -latest -find VC\Tools\LLVM\**\bin\clang-format.exe') do ( 
  echo clang-format found : %%i 
   %%i --version 
  set "clang_format="%%i""
)

for /r %%f in ( *.cc *.cpp *.hpp *.h *.c ) do (
  set file_path=%%~pf
    
  :: discard the dependencies directory
  if /I "!file_path!"=="!file_path:dependencies%=!" (
     echo formatting [%%f]
     call !clang_format! -i -style=file "%%f" 
  )
)

endlocal

echo Finish
pause
