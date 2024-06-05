# BLE_THSensor
BLE temperature and humidity sensor based on [TS0201](https://pvvx.github.io/TS0201_TZ3000/), [TH03Z](https://pvvx.github.io/TH03Z/)

<img src="https://pvvx.github.io/TS0201_TZ3000/img/ts0201_.jpg" width="120"/> <img src="https://pvvx.github.io/TH03Z/img/TH03Z.jpg" width="200"/>

* Варианты Zigbee firmware находится в [ZigbeeTLc](https://github.com/pvvx/ZigbeeTLc)

[ZTH01](https://pvvx.github.io/TS0601_TZE200_zth01/), [ZTH02](https://pvvx.github.io/TS0601_TZE200_zth02/) работают с доработкой - [требуется выпаять лишнюю микросхему и впаять перемычки](https://github.com/pvvx/BLE_THSensor/issues/13#issuecomment-2148975619).

<img src="https://pvvx.github.io/TS0601_TZE200_zth01/img/ZTH01.jpg" width="120"/> <img src="https://pvvx.github.io/TS0601_TZE200_zth02/img/zth02.jpg" width="120"/> 

## Основные характеристики

* Поддерживаемые форматы BLE рекламы: BTHome v1, BTHome v2, PVVX, MiHome.
* Поддержка Bluetooth ver 5 c LE Long Range.
* Объем памяти замеров 51943 шт. [GraphMemo.html](https://pvvx.github.io/ATC_MiThermometer/GraphMemo.html)
* Среднее потребление зависит от настроек - менее 20 мкА. TH03Z с установленным интервалом рекламы на 5 сек, TX RF +0 дБм потребляет до 8 мкА.

## Функции кнопки

Кратковременное нажатие кнопки перключает BLE рекламу в режим Legacy (BT4.2) на 80 секунд с Advertising interval в 1 сек.
Используется для быстрого соединения и если включен режим BLE LongRange или установлен большой Advertising interval.
Нажатие кнопки сопровождается включением светодиода.

## Hастройки 

Для настройки BLE версий используется [TelinkMiFlasher.html](https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html).

## Первоначальная прошивка устройства

Первоначально перепрошить устройство возможно двумя вариантами.

1. С помощью Zigbee OTA файлов [1141-d3a3-00993001](https://github.com/pvvx/BLE_THSensor/tree/master/source/TS0201/bin)-xxxx.zigbee в вариант BLE или с помощью файлов [1141-d3a3-01993001](https://github.com/pvvx/ZigbeeTLc/tree/master/bin)-xxxx.zigbee в [ZigbeeTLc](https://github.com/pvvx/ZigbeeTLc).
2. USB-COM программатором через выводы разъема SWS и RST с файлами [xxx.bin](https://github.com/pvvx/BLE_THSensor/tree/master/source/TS0201/bin). При первом программировании из варианта Tuya желательно стереть всю Flash.

После прошивки в вариант BLE должно появиться устройство с именем "THS_xxxxxx", где "xxxxxx" - три последних байта MAC в hex виде.

Далее в [TelinkMiFlasher.html](https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html) возможно прошить любую версию, включая Zigbee.

Файлы прошивки для восстановления из ZigbeeTlc в BLE: [1141-02xx-99993001-xxxxxxxxxx.zigbee](https://github.com/pvvx/BLE_THSensor/tree/master/source/TS0201/bin)

Самый простой программатор-эмулятор Telink-Swire без обратной связи: [WebUSBCOMflasher](https://pvvx.github.io/ATC_MiThermometer/USBCOMFlashTx.html)

![pgm](https://raw.githubusercontent.com/pvvx/BLE_THSensor/master/img/ts0201pgm.png)

Более сложный программатор-эмулятор Telink-Swire с переходом на UART связь: [TlsrComProg825x](https://github.com/pvvx/TlsrComProg825x)

Полнофункциональный аппаратный Telink-Swire программатор: [TLSRPGM](https://github.com/pvvx/TLSRPGM) 

---

Работа в морозилке холодильника (BTHome v2, BLE LongRange для стабильной дальней связи через железные стенки холодильника):

<img src="https://github.com/pvvx/BLE_THSensor/blob/master/img/ha_fridge.jpg"/>

