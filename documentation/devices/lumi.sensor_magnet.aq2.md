# Aquara Window and Door Sensor (MCCGQ11LM )
## Image
![Photo](https://xiaomi-mi.com/uploads/CatalogueImage/xiaomi-aqara-window-door-sensor-02_15779_1506342091.jpg)
## Example reports
```
AqaraHub/report/158D0001B14707/1/Basic/ModelIdentifier: {"type":"string","value":"lumi.sensor_magnet.aq2"}
AqaraHub/report/158D0001B14707/1/OnOff/OnOff: {"type":"bool","value":true}
AqaraHub/report/158D0001B14707/1/OnOff/OnOff: {"type":"bool","value":false}

AqaraHub/report/158D0001B14707/1/Basic/0xFF01: {"type":"xiaomi_ff01","value":{"1":{"type":"uint16","value":2995},"10":{"type":"uint16","value":0},"100":{"type":"bool","value":false},"3":{"type":"int8","value":30},"4":{"type":"uint16","value":17320},"5":{"type":"uint16","value":61},"6":{"type":"uint40","value":2}}}
```
### Attributes
### Basic/ModelIdentifier
string "lumi.sensor_magnet.aq2"
### Basic/ApplicationVersion
uint8 10
### Basic/0xFF01
Not sure yet, battery amongst others.
### OnOff/OnOff
false when together (window closed), true when apart (window open).
