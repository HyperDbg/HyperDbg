@echo off

set LLVM_PATH="c:\Program Files\LLVM\bin"

cls 
 
setlocal enableextensions enabledelayedexpansion
  
for /r %%f in ( *.cpp *.hpp *.h *.c ) do (
  set file_path=%%~pf
    
  :: discard the dependencies directory
  if /I "!file_path!"=="!file_path:dependencies%=!" (
     echo formatting [%%f]

     call %LLVM_PATH%\clang-format.exe -i -style=file "%%f" 
  )
)

endlocal

echo Finish
pause