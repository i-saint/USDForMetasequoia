set BASE_DIR=%~dp0..\
set SRC_DIR32=%BASE_DIR%_out\Win32_Debug
set SRC_DIR64=%BASE_DIR%_out\x64_Debug
set EXT_DIR64=%BASE_DIR%Externals\x64\lib
set DST_IMPORT32=%APPDATA%\tetraface\Metasequoia4\Plugins\Import
set DST_IMPORT64=%APPDATA%\tetraface\Metasequoia4_x64\Plugins\Import
set DST_EXPORT32=%APPDATA%\tetraface\Metasequoia4\Plugins\Export
set DST_EXPORT64=%APPDATA%\tetraface\Metasequoia4_x64\Plugins\Export
set DST_STATION32=%APPDATA%\tetraface\Metasequoia4\Plugins\Station
set DST_STATION64=%APPDATA%\tetraface\Metasequoia4_x64\Plugins\Station
set USDCORE_DIR32=%DST_STATION32%\mqusdCore
set USDCORE_DIR64=%DST_STATION64%\mqusdCore

mkdir "%DST_IMPORT32%"
mkdir "%DST_IMPORT64%"
mkdir "%DST_EXPORT32%"
mkdir "%DST_EXPORT64%"
mkdir "%DST_STATION32%"
mkdir "%DST_STATION64%"

del "%DST_STATION32%\mudbg.dll"
del "%DST_STATION32%\mqabc.dll"
del "%DST_STATION32%\mqusd.dll"
del "%DST_IMPORT32%\mqabcImport.dll"
del "%DST_IMPORT32%\mqusdImport.dll"
del "%DST_EXPORT32%\mqabcExport.dll"
del "%DST_EXPORT32%\mqusdExport.dll"
mklink "%DST_STATION32%\mudbg.dll" "%SRC_DIR32%\mudbg.dll"
mklink "%DST_STATION32%\mqabc.dll" "%SRC_DIR32%\470\mqabc.dll"
mklink "%DST_STATION32%\mqusd.dll" "%SRC_DIR32%\470\mqusd.dll"
mklink "%DST_IMPORT32%\mqabcImport.dll" "%SRC_DIR32%\470\mqabcImport.dll"
mklink "%DST_IMPORT32%\mqusdImport.dll" "%SRC_DIR32%\470\mqusdImport.dll"
mklink "%DST_EXPORT32%\mqabcExport.dll" "%SRC_DIR32%\470\mqabcExport.dll"
mklink "%DST_EXPORT32%\mqusdExport.dll" "%SRC_DIR32%\470\mqusdExport.dll"
IF NOT EXIST "%USDCORE_DIR32%" (
    mkdir "%USDCORE_DIR32%"
    mklink "%USDCORE_DIR32%\SceneGraphUSD.exe" "%SRC_DIR64%\SceneGraphUSD.exe"
    mklink "%USDCORE_DIR32%\SceneGraphUSD.dll" "%SRC_DIR64%\SceneGraphUSD.dll"
    mklink "%USDCORE_DIR32%\tbb.dll" "%EXT_DIR64%\tbb.dll"
    mklink "%USDCORE_DIR32%\usd_ms.dll" "%EXT_DIR64%\usd_ms.dll"
    mklink /j "%USDCORE_DIR32%\usd" "%EXT_DIR64%\usd"
)

del "%DST_STATION64%\mudbg.dll"
del "%DST_STATION64%\mqabc.dll"
del "%DST_STATION64%\mqusd.dll"
del "%DST_IMPORT64%\mqabcImport.dll"
del "%DST_IMPORT64%\mqusdImport.dll"
del "%DST_EXPORT64%\mqabcExport.dll"
del "%DST_EXPORT64%\mqusdExport.dll"
mklink "%DST_STATION64%\mudbg.dll" "%SRC_DIR64%\mudbg.dll"
mklink "%DST_STATION64%\mqabc.dll" "%SRC_DIR64%\470\mqabc.dll"
mklink "%DST_STATION64%\mqusd.dll" "%SRC_DIR64%\470\mqusd.dll"
mklink "%DST_IMPORT64%\mqabcImport.dll" "%SRC_DIR64%\470\mqabcImport.dll"
mklink "%DST_IMPORT64%\mqusdImport.dll" "%SRC_DIR64%\470\mqusdImport.dll"
mklink "%DST_EXPORT64%\mqabcExport.dll" "%SRC_DIR64%\470\mqabcExport.dll"
mklink "%DST_EXPORT64%\mqusdExport.dll" "%SRC_DIR64%\470\mqusdExport.dll"
IF NOT EXIST "%USDCORE_DIR64%" (
    mkdir "%USDCORE_DIR64%"
    mklink "%USDCORE_DIR64%\SceneGraphUSD.dll" "%SRC_DIR64%\SceneGraphUSD.dll"
    mklink "%USDCORE_DIR64%\tbb.dll" "%EXT_DIR64%\tbb.dll"
    mklink "%USDCORE_DIR64%\usd_ms.dll" "%EXT_DIR64%\usd_ms.dll"
    mklink /j "%USDCORE_DIR64%\usd" "%EXT_DIR64%\usd"
)
