@Path=E:\Telink\SDK;E:\Telink\SDK\jre\bin;E:\Telink\SDK\opt\tc32\tools;E:\Telink\SDK\opt\tc32\bin;E:\Telink\SDK\usr\bin;E:\Telink\SDK\bin;%PATH%
@set SWVER=_v45
@del /Q .\bin
make -s -j PROJECT_NAME=TS0201%SWVER% POJECT_DEF="-DDEVICE_TYPE=DEVICE_TS0201"
@if not exist "bin\TS0201%SWVER%.bin" goto :error
make -s -j PROJECT_NAME=TH03Z%SWVER% POJECT_DEF="-DDEVICE_TYPE=DEVICE_TH03Z"
@if not exist "bin\TH03Z%SWVER%.bin" goto :error
@exit
:error
echo "Error!"

         