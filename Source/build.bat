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

    set SRC_MQ32=out\Win32_Release\%MQ_VERSION%
    set DIST_DIR32=_dist\mqusd_Windows_32bit
    set DIST_IMPORT32=%DIST_DIR32%\Import
    set DIST_EXPORT32=%DIST_DIR32%\Export
    set DIST_STATION32=%DIST_DIR32%\Station
    set DIST_MISC32=%DIST_DIR32%\Misc
    mkdir "%DIST_IMPORT32%"
    mkdir "%DIST_EXPORT32%"
    mkdir "%DIST_STATION32%"
    mkdir "%DIST_MISC32%\mqusd"
    copy %SRC_MQ32%\mqabcImporter.dll "%DIST_IMPORT32%"
    copy %SRC_MQ32%\mqusdImporter.dll "%DIST_IMPORT32%"
    copy %SRC_MQ32%\mqabcExporter.dll "%DIST_EXPORT32%"
    copy %SRC_MQ32%\mqusdExporter.dll "%DIST_EXPORT32%"
    copy %SRC_MQ32%\mqabcRecorder.dll "%DIST_STATION32%"
    copy %SRC_MQ32%\mqusdRecorder.dll "%DIST_STATION32%"
    copy %SRC_MQ32%\mqabc.dll "%DIST_MISC32%"
    copy %SRC_MQ32%\mqusd.dll "%DIST_MISC32%"
    copy _out\x64_Release\SceneGraphUSD.dll "%DIST_MISC32%\mqusd"
    copy _out\x64_Release\SceneGraphUSD.exe "%DIST_MISC32%\mqusd"
    copy Externals\x64\lib\tbb.dll "%DIST_MISC32%\mqusd"
    copy Externals\x64\lib\usd_ms.dll "%DIST_MISC32%\mqusd"
    xcopy /EIY Externals\x64\lib\usd "%DIST_MISC32%\mqusd\usd"

    set SRC_MQ64=_out\x64_Release\%MQ_VERSION%
    set DIST_DIR64=_dist\mqusd_Windows_64bit
    set DIST_IMPORT64=%DIST_DIR64%\Import
    set DIST_EXPORT64=%DIST_DIR64%\Export
    set DIST_STATION64=%DIST_DIR64%\Station
    set DIST_MISC64=%DIST_DIR64%\Misc
    mkdir "%DIST_IMPORT64%"
    mkdir "%DIST_EXPORT64%"
    mkdir "%DIST_STATION64%"
    mkdir "%DIST_MISC64%\mqusd"
    copy %SRC_MQ64%\mqabcImporter.dll "%DIST_IMPORT64%"
    copy %SRC_MQ64%\mqusdImporter.dll "%DIST_IMPORT64%"
    copy %SRC_MQ64%\mqabcExporter.dll "%DIST_EXPORT64%"
    copy %SRC_MQ64%\mqusdExporter.dll "%DIST_EXPORT64%"
    copy %SRC_MQ64%\mqabcRecorder.dll "%DIST_STATION64%"
    copy %SRC_MQ64%\mqusdRecorder.dll "%DIST_STATION64%"
    copy %SRC_MQ64%\mqabc.dll "%DIST_MISC64%"
    copy %SRC_MQ64%\mqusd.dll "%DIST_MISC64%"
    copy _out\x64_Release\SceneGraphUSD.dll "%DIST_MISC64%\mqusd"
    copy Externals\x64\lib\tbb.dll "%DIST_MISC64%\mqusd"
    copy Externals\x64\lib\usd_ms.dll "%DIST_MISC64%\mqusd"
    xcopy /EIY Externals\x64\lib\usd "%DIST_MISC64%\mqusd\usd"

    copy Scripts\install_Windows.bat _dist\
    copy Scripts\install_Mac.sh _dist\
    exit /B 0
