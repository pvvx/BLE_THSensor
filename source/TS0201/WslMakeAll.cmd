@set SWVER=_v45
@del /Q .\bin
wsl make -s PROJECT_NAME=TS0201%SWVER% POJECT_DEF="-DDEVICE_TYPE=DEVICE_TS0201"
@if not exist ".\bin\TS0201%SWVER%.bin" goto :error
@exit
:error
echo "Error!"

         