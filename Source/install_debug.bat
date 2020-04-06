set BASE_DIR=%~dp0
set SRC_DIR32="%BASE_DIR%_out\Win32_Debug"
set SRC_DIR64="%BASE_DIR%_out\x64_Debug"
set EXT_DIR64="%BASE_DIR%Externals\x64\lib"
set DST_DIR32="%APPDATA%\tetraface\Metasequoia4\Plugins\Station"
set DST_DIR64="%APPDATA%\tetraface\Metasequoia4_x64\Plugins\Station"
set USDCORE_DIR32="%DST_DIR32%\mqusdCore"
set USDCORE_DIR64="%DST_DIR64%\mqusdCore"

echo %SRC_DIR32%
del "%DST_DIR32%\mqabc.dll"
del "%DST_DIR32%\mqusd.dll"
mklink "%DST_DIR32%\mqabc.dll" "%SRC_DIR32%\470\mqabc.dll"
mklink "%DST_DIR32%\mqusd.dll" "%SRC_DIR32%\470\mqusd.dll"
IF NOT EXIST "%USDCORE_DIR32%" (
    mkdir %USDCORE_DIR32%
    mklink "%USDCORE_DIR32%\SceneGraphUSD.exe" "%SRC_DIR64%\SceneGraphUSD.exe"
    mklink "%USDCORE_DIR32%\SceneGraphUSD.dll" "%SRC_DIR64%\SceneGraphUSD.dll"
    mklink "%USDCORE_DIR32%\tbb.dll" "%EXT_DIR64%\tbb.dll"
    mklink "%USDCORE_DIR32%\usd_ms.dll" "%EXT_DIR64%\usd_ms.dll"
    xcopy /EIY "%EXT_DIR64%\usd" "%USDCORE_DIR32%\usd"
)

del "%DST_DIR64%\mqabc.dll"
del "%DST_DIR64%\mqusd.dll"
mklink "%DST_DIR64%\mqabc.dll" "%SRC_DIR64%\470\mqabc.dll"
mklink "%DST_DIR64%\mqusd.dll" "%SRC_DIR64%\470\mqusd.dll"
IF NOT EXIST "%USDCORE_DIR64%" (
    mkdir %USDCORE_DIR64%
    mklink "%USDCORE_DIR64%\SceneGraphUSD.dll" "%SRC_DIR64%\SceneGraphUSD.dll"
    mklink "%USDCORE_DIR64%\tbb.dll" "%EXT_DIR64%\tbb.dll"
    mklink "%USDCORE_DIR64%\usd_ms.dll" "%EXT_DIR64%\usd_ms.dll"
    mklink /j "%USDCORE_DIR64%\usd" "%EXT_DIR64%\usd"
    rem xcopy /EIY "%EXT_DIR64%\usd" "%USDCORE_DIR64%\usd"
)
