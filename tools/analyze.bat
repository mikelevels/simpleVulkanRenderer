@echo off
rem tools\analyze.bat: static analysis of this project. Run from anywhere.
rem   clang-tidy  (checks from ..\.clang-tidy, plus -Wdocumentation for the /// comments)
rem   Cppcheck    (style, performance, warning, portability)
rem   lizard      (functions over 25 lines of code)
rem   Doxygen     (every declaration documented; warnings fail the run)
rem Results go to build-analyze\reports\ and build-docs\. Exit code 0 = all clean.
rem See MyNotes\analysis-2026-10-15.md.
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
set P=%~dp0..
set B=%P%\build-analyze
set R=%B%\reports
set FAILED=0
rem Prefer the newer LLVM 23 clang-tidy (its -Wdocumentation is stricter);
rem fall back to the one that comes with Visual Studio (LLVM 19).
set LLVM=C:\Users\gonza\Dev\Tools\llvm-23.1.3\bin
if not exist "%LLVM%\clang-tidy.exe" set LLVM=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin
rem Ninja + compile_commands.json: clang-tidy reads the compiler flags from it.
rem Module scanning off: its @modmap flags confuse clang-tidy.
cmake --fresh -S "%P%" -B "%B%" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_SCAN_FOR_MODULES=OFF >nul 2>&1
if not exist "%R%" mkdir "%R%"
if exist "%R%\clang-tidy.txt" del "%R%\clang-tidy.txt"

echo [clang-tidy]
for /r "%P%\src" %%F in (*.cpp) do (
  "%LLVM%\clang-tidy.exe" -p "%B%" --extra-arg=-Wdocumentation "%%F" >>"%R%\clang-tidy.txt" 2>nul
)
rem findstr succeeds (errorlevel 0) when it FINDS a warning, which is a failure here.
findstr /c:"warning:" /c:"error:" "%R%\clang-tidy.txt" && set FAILED=1

echo [Cppcheck]
rem --error-exitcode=1: Cppcheck exits 0 even when it finds problems unless told otherwise.
"C:\Program Files\Cppcheck\cppcheck.exe" --error-exitcode=1 --enable=style,performance,warning,portability --inline-suppr --quiet --std=c++20 -I "%P%\src" "%P%\src" 2>"%R%\cppcheck.txt" || set FAILED=1
type "%R%\cppcheck.txt"

echo [lizard: functions over 25 lines of code]
python -m lizard "%P%\src" -Tnloc=25 -w >"%R%\lizard-warnings.txt" || set FAILED=1
type "%R%\lizard-warnings.txt"

echo [Doxygen]
pushd "%P%"
"C:\Users\gonza\Dev\Tools\doxygen-1.18.0\doxygen.exe" Doxyfile >nul 2>&1 || set FAILED=1
popd
type "%P%\build-docs\doxygen-warnings.txt"

if %FAILED%==0 (echo ALL CLEAN) else (echo FINDINGS ABOVE)
exit /b %FAILED%
