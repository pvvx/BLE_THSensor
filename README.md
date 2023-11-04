# BLE_THSensor
BLE temperature and humidity sensor based on [TS0201](https://pvvx.github.io/TS0201)

<img src="https://pvvx.github.io/TS0201/img/ts0201.jpg" width="120"/>

Прошивается программатором через выводы разъема SWS и RST. См. [TS0201](https://pvvx.github.io/TS0201).

Самый простой программатор-эмулятор Telink-Swire без обратной связи: [WebUSBCOMflasher](https://pvvx.github.io/ATC_MiThermometer/USBCOMFlashTx.html)

![pgm](https://raw.githubusercontent.com/pvvx/BLE_THSensor/master/img/ts0201pgm.png)

Файл прошивки: [TS0201_v45.bin](https://github.com/pvvx/BLE_THSensor/raw/master/source/TS0201/TS0201_v45.bin)

Более сложный программатор-эмулятор Telink-Swire с переходом на UART связь: [TlsrComProg825x](https://github.com/pvvx/TlsrComProg825x)

Полнофункциональный аппаратный Telink-Swire программатор: [TLSRPGM](https://github.com/pvvx/TLSRPGM) 

Более сложный программатор-эмулятор Telink-Swire с переходом на UART связь: [TlsrComProg825x](https://github.com/pvvx/TlsrComProg825x)

Полнофункциональный аппаратный Telink-Swire программатор: [TLSRPGM](https://github.com/pvvx/TLSRPGM) 

Для настройки используйте [TelinkMiFlasher.html](https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html) от версии 6.5.

Объем памяти замеров увеличен до 51943 шт. [GraphMemo.html](https://pvvx.github.io/ATC_MiThermometer/GraphMemo.html)

Поддерживаемые форматы рекламы: BTHome v1, BTHome v2, PVVX, MiHome.

Кнопка включает BLE рекламу в режим Legacy (BT4.2) на 80 секунд с Advertising interval в 1 сек для соединения. 
Используется если включен режим BLE LongRange или установлен большой Advertising interval.


Работа в морозилке холодильника:

<img src="https://github.com/pvvx/BLE_THSensor/blob/master/img/ha_fridge.jpg"/>

Среднее потребление менее 20 мкА - зависит от настроек.

(Потребление оригинального TS0201 в ZigBee - более 0.1 мА при связи со шлюзом, без подстроек интервалов)

