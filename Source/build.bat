setlocal
cd /d "%~dp0"
call toolchain.bat
call :DoBuild 470
exit /B 0

:DoBuild
    set MQ_VERSION=%~1
    set MQ_SDK_DIR=%cd%\Externals\mqsdk%MQ_VERSION%\mqsdk
    msbuild USDForMetasequoia.sln /t:Build /p:Configuration=Release /p:Platform=x64 /m /nologo
    IF %ERRORLEVEL% NEQ 0 (
        pause
        exit /B 1
    )
    msbuild USDForMetasequoia.sln /t:Build /p:Configuration=Release /p:Platform=Win32 /m /nologo
    IF %ERRORLEVEL% NEQ 0 (
        pause
        exit /B 1
    )

    set DIST_DIR32="_dist\mqusd_Windows_32bit"
    set DIST_IMPORT32="%DIST_DIR32%\Import"
    set DIST_EXPORT32="%DIST_DIR32%\Export"
    set DIST_STATION32="%DIST_DIR32%\Station"
    set CORE_DIR32="%DIST_STATION32%\mqusdCore"
    mkdir "%DIST_IMPORT32%"
    mkdir "%DIST_EXPORT32%"
    mkdir "%DIST_STATION32%"
    mkdir "%CORE_DIR32%"
    copy _out\Win32_Release\%MQ_VERSION%\mqabc.dll "%DIST_STATION32%"
    copy _out\Win32_Release\%MQ_VERSION%\mqusdImport.dll "%DIST_IMPORT32%"
    copy _out\Win32_Release\%MQ_VERSION%\mqusdExport.dll "%DIST_EXPORT32%"
    copy _out\Win32_Release\%MQ_VERSION%\mqusd.dll "%DIST_STATION32%"
    copy _out\x64_Release\SceneGraphUSD.dll "%CORE_DIR32%"
    copy _out\x64_Release\SceneGraphUSD.exe "%CORE_DIR32%"
    copy Externals\x64\lib\tbb.dll "%CORE_DIR32%"
    copy Externals\x64\lib\usd_ms.dll "%CORE_DIR32%"
    xcopy /EIY Externals\x64\lib\usd "%CORE_DIR32%\usd"

    set DIST_DIR64="_dist\mqusd_Windows_64bit"
    set DIST_IMPORT64="%DIST_DIR64%\Import"
    set DIST_EXPORT64="%DIST_DIR64%\Export"
    set DIST_STATION64="%DIST_DIR64%\Station"
    set CORE_DIR64="%DIST_STATION64%\mqusdCore"
    mkdir "%DIST_IMPORT64%"
    mkdir "%DIST_EXPORT64%"
    mkdir "%DIST_STATION64%"
    mkdir "%CORE_DIR64%"
    copy _out\x64_Release\%MQ_VERSION%\mqabc.dll "%DIST_STATION64%"
    copy _out\x64_Release\%MQ_VERSION%\mqusdImport.dll "%DIST_IMPORT64%"
    copy _out\x64_Release\%MQ_VERSION%\mqusdExport.dll "%DIST_EXPORT64%"
    copy _out\x64_Release\%MQ_VERSION%\mqusd.dll "%DIST_STATION64%"
    copy _out\x64_Release\SceneGraphUSD.dll "%CORE_DIR64%"
    copy Externals\x64\lib\tbb.dll "%CORE_DIR64%"
    copy Externals\x64\lib\usd_ms.dll "%CORE_DIR64%"
    xcopy /EIY Externals\x64\lib\usd "%CORE_DIR64%\usd"
    exit /B 0
