set BASE_DIR=%~dp0..\
set SRC_DIR32=%BASE_DIR%_out\Win32_Debug
set SRC_DIR64=%BASE_DIR%_out\x64_Debug
set EXT_DIR64=%BASE_DIR%Externals\x64\lib

set PLUGINS_DIR32=%APPDATA%\tetraface\Metasequoia4\Plugins
set PLUGINS_DIR64=%APPDATA%\tetraface\Metasequoia4_x64\Plugins
set DST_IMPORT32=%PLUGINS_DIR32%\Import
set DST_IMPORT64=%PLUGINS_DIR64%\Import
set DST_EXPORT32=%PLUGINS_DIR32%\Export
set DST_EXPORT64=%PLUGINS_DIR64%\Export
set DST_STATION32=%PLUGINS_DIR32%\Station
set DST_STATION64=%PLUGINS_DIR64%\Station
set DST_MISC32=%PLUGINS_DIR32%\Misc
set DST_MISC64=%PLUGINS_DIR64%\Misc


del "%DST_IMPORT32%\mqabcImport.dll"
del "%DST_IMPORT32%\mqusdImport.dll"
del "%DST_EXPORT32%\mqabcExport.dll"
del "%DST_EXPORT32%\mqusdExport.dll"
del "%DST_STATION32%\mudbg.dll"
del "%DST_STATION32%\mqabc.dll"
del "%DST_STATION32%\mqusd.dll"
del "%DST_MISC32%\mqusd\SceneGraphUSD.exe"
del "%DST_MISC32%\mqusd\SceneGraphUSD.dll"
del "%DST_MISC32%\mqusd\tbb.dll"
del "%DST_MISC32%\mqusd\usd_ms.dll"

mkdir "%DST_IMPORT32%"
mkdir "%DST_EXPORT32%"
mkdir "%DST_STATION32%"
mkdir "%DST_MISC32%"
mkdir "%DST_MISC32%\usd"

mklink "%DST_IMPORT32%\mqabcImport.dll" "%SRC_DIR32%\470\mqabcImport.dll"
mklink "%DST_IMPORT32%\mqusdImport.dll" "%SRC_DIR32%\470\mqusdImport.dll"
mklink "%DST_EXPORT32%\mqabcExport.dll" "%SRC_DIR32%\470\mqabcExport.dll"
mklink "%DST_EXPORT32%\mqusdExport.dll" "%SRC_DIR32%\470\mqusdExport.dll"
mklink "%DST_EXPORT32%\mqabcRecorder.dll" "%SRC_DIR32%\470\mqabcRecorder.dll"
mklink "%DST_EXPORT32%\mqusdRecorder.dll" "%SRC_DIR32%\470\mqusdRecorder.dll"
mklink "%DST_STATION32%\mudbg.dll" "%SRC_DIR32%\mudbg.dll"
mklink "%DST_STATION32%\mqabc.dll" "%SRC_DIR32%\470\mqabc.dll"
mklink "%DST_STATION32%\mqusd.dll" "%SRC_DIR32%\470\mqusd.dll"
mklink "%DST_MISC32%\mqusd\SceneGraphUSD.exe" "%SRC_DIR64%\SceneGraphUSD.exe"
mklink "%DST_MISC32%\mqusd\SceneGraphUSD.dll" "%SRC_DIR64%\SceneGraphUSD.dll"
mklink "%DST_MISC32%\mqusd\tbb.dll" "%EXT_DIR64%\tbb.dll"
mklink "%DST_MISC32%\mqusd\usd_ms.dll" "%EXT_DIR64%\usd_ms.dll"
mklink /j "%DST_MISC32%\mqusd\usd" "%EXT_DIR64%\usd"


del "%DST_IMPORT64%\mqabcImport.dll"
del "%DST_IMPORT64%\mqusdImport.dll"
del "%DST_EXPORT64%\mqabcExport.dll"
del "%DST_EXPORT64%\mqusdExport.dll"
del "%DST_STATION64%\mudbg.dll"
del "%DST_STATION64%\mqabc.dll"
del "%DST_STATION64%\mqusd.dll"
del "%DST_MISC64%\mqusd\SceneGraphUSD.dll"
del "%DST_MISC64%\mqusd\tbb.dll"
del "%DST_MISC64%\mqusd\usd_ms.dll"

mkdir "%DST_IMPORT64%"
mkdir "%DST_EXPORT64%"
mkdir "%DST_STATION64%"
mkdir "%DST_MISC64%"
mkdir "%DST_MISC64%\usd"

mklink "%DST_IMPORT64%\mqabcImport.dll" "%SRC_DIR64%\470\mqabcImport.dll"
mklink "%DST_IMPORT64%\mqusdImport.dll" "%SRC_DIR64%\470\mqusdImport.dll"
mklink "%DST_EXPORT64%\mqabcExport.dll" "%SRC_DIR64%\470\mqabcExport.dll"
mklink "%DST_EXPORT64%\mqusdExport.dll" "%SRC_DIR64%\470\mqusdExport.dll"
mklink "%DST_EXPORT64%\mqabcRecorder.dll" "%SRC_DIR64%\470\mqabcRecorder.dll"
mklink "%DST_EXPORT64%\mqusdRecorder.dll" "%SRC_DIR64%\470\mqusdRecorder.dll"
mklink "%DST_MISC64%\mudbg.dll" "%SRC_DIR64%\mudbg.dll"
mklink "%DST_MISC64%\mqabc.dll" "%SRC_DIR64%\470\mqabc.dll"
mklink "%DST_MISC64%\mqusd.dll" "%SRC_DIR64%\470\mqusd.dll"
mklink "%DST_MISC64%\mqusd\SceneGraphUSD.dll" "%SRC_DIR64%\SceneGraphUSD.dll"
mklink "%DST_MISC64%\mqusd\tbb.dll" "%EXT_DIR64%\tbb.dll"
mklink "%DST_MISC64%\mqusd\usd_ms.dll" "%EXT_DIR64%\usd_ms.dll"
mklink /j "%DST_MISC64%\mqusd\usd" "%EXT_DIR64%\usd"
