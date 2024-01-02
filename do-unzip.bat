@echo off
if x%2==x (
    echo Usage: do-unzip ^<file.zip^> ^<directory^>
    exit /b 1
)
where /Q tar.exe
if x%ERRORLEVEL%==x0 (
    @echo on
    if not exist %2 mkdir %2
    tar.exe -x -C %2 -f %1
    exit /b 0
) else (
    echo Error: tar.exe not found
    exit /b 1
)
