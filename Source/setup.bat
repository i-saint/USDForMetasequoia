@echo off

cd Externals

IF NOT EXIST "7za.exe" (
    echo "downloading 7za.exe..."
    powershell.exe -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "[System.Net.ServicePointManager]::SecurityProtocol=[System.Net.SecurityProtocolType]::Tls12; wget https://github.com/i-saint/AlembicForMetasequoia/releases/download/libraries/7za.exe -OutFile 7za.exe"
)

echo "downloading external libararies..."
powershell.exe -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "[System.Net.ServicePointManager]::SecurityProtocol=[System.Net.SecurityProtocolType]::Tls12; wget https://github.com/i-saint/AlembicForMetasequoia/releases/download/libraries/Externals.7z -OutFile Externals.7z"
7za.exe x -aos Externals.7z

IF NOT EXIST "mqsdk470.zip" (
    echo "downloading mqsdk470.zip ..."
    powershell.exe -Command "wget http://www.metaseq.net/metaseq/mqsdk470.zip -OutFile mqsdk470.zip"
7za.exe x -aos -o"mqsdk470" mqsdk470.zip
)

cd ..
