@echo off
rem tools\asan.bat: build the app with AddressSanitizer (finds out-of-bounds
rem reads/writes, use-after-free, leaks of some kinds) into build-asan\.
rem Run build-asan\App.exe afterwards; any report goes to the console (stderr)
rem and stops the program. See MyNotes\analysis-2026-10-15.md.
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
set P=%~dp0..
set B=%P%\build-asan
cmake --fresh -S "%P%" -B "%B%" -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_CXX_FLAGS=/fsanitize=address /EHsc /Zi" "-DCMAKE_C_FLAGS=/fsanitize=address /Zi" -DCMAKE_CXX_SCAN_FOR_MODULES=OFF
cmake --build "%B%"
rem The ASan runtime DLL must sit next to the exe (or be on PATH).
copy /y "%VCToolsInstallDir%bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll" "%B%\" >nul
echo Built %B%\App.exe
