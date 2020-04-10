set BASE_DIR=%~dp0
set SRC_DIR32=%BASE_DIR%mqusd_Windows_32bit
set SRC_DIR64=%BASE_DIR%mqusd_Windows_64bit
set MQ_DIR32=%APPDATA%\tetraface\Metasequoia4
set MQ_DIR64=%APPDATA%\tetraface\Metasequoia4_x64

IF EXIST "%MQ_DIR32%" (
    xcopy /EIY "%SRC_DIR32%\*" "%MQ_DIR32%\Plugins"
)
IF EXIST "%MQ_DIR64%" (
    xcopy /EIY "%SRC_DIR64%\*" "%MQ_DIR64%\Plugins"
)