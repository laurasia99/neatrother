@echo off
if x%2==x (
    echo Usage: do-curl ^<out-file^> ^<url^>
    exit /b 1
)
where /Q curl.exe
if x%ERRORLEVEL%==x0 (
    curl.exe -o %1 %2
    exit /b 0
) else (
    echo Error: curl.exe not found
    exit /b 1
)
