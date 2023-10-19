# BLE_THSensor
BLE temperature and humidity sensor based on [TS0201](https://pvvx.github.io/TS0201)

Прошивается программатором через выводы разъема SWS и RST. См. [TS0201](https://pvvx.github.io/TS0201).

Файл прошивки: [TS0201_v44.bin](https://github.com/pvvx/BLE_THSensor/raw/main/source/TS0201/TS0201_v44.bin)

Для настройки используйте [TelinkMiFlasher.html](https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html) от версии 6.5.

Поддерживаемые форматы рекламы: BTHome v1, BTHome v2, PVVX, MiHome.

Кнопка включает BLE рекламу в режим Legacy (BT4.2) на 80 секунд с Advertising interval в 1 сек для соединения. 
Используется если включен режим BLE LongRange или установлен большой Advertising interval.

