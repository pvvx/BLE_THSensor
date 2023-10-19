@set SWVER=_v44
@del /Q "TS0201%SWVER%.bin"
wsl make -s PROJECT_NAME=TS0201%SWVER% POJECT_DEF="-DDEVICE_TYPE=DEVICE_TS0201"
@if not exist "TS0201%SWVER%.bin" goto :error
@exit
:error
echo "Error!"

         