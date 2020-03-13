setlocal
cd /d "%~dp0"
call toolchain.bat
call :DoBuild 470
exit /B 0

:DoBuild
    set MQ_VERSION=%~1
    set MQ_SDK_DIR=%cd%\Externals\mqsdk%MQ_VERSION%\mqsdk
    msbuild mqabcPlayer.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64 /m /nologo
    IF %ERRORLEVEL% NEQ 0 (
        pause
        exit /B 1
    )
    msbuild mqabcRecorder.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64 /m /nologo
    IF %ERRORLEVEL% NEQ 0 (
        pause
        exit /B 1
    )
    msbuild mqabcPlayer.vcxproj /t:Build /p:Configuration=Release /p:Platform=Win32 /m /nologo
    IF %ERRORLEVEL% NEQ 0 (
        pause
        exit /B 1
    )
    msbuild mqabcRecorder.vcxproj /t:Build /p:Configuration=Release /p:Platform=Win32 /m /nologo
    IF %ERRORLEVEL% NEQ 0 (
        pause
        exit /B 1
    )
    set DIST_DIR64="_dist\mqabc_Windows_64bit"
    set DIST_DIR32="_dist\mqabc_Windows_32bit"
    mkdir "%DIST_DIR64%"
    mkdir "%DIST_DIR32%"
    copy _out\x64_Release\mqabcPlayer%MQ_VERSION%\mqabcPlayer.dll "%DIST_DIR64%"
    copy _out\x64_Release\mqabcRecorder%MQ_VERSION%\mqabcRecorder.dll "%DIST_DIR64%"
    copy _out\Win32_Release\mqabcPlayer%MQ_VERSION%\mqabcPlayer.dll "%DIST_DIR32%"
    copy _out\Win32_Release\mqabcRecorder%MQ_VERSION%\mqabcRecorder.dll "%DIST_DIR32%"
    exit /B 0
