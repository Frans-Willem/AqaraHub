# Xiaomi Mi Smart Home Door / Window Sensors
## Image
![Photo](https://xiaomi-mi.com/uploads/CatalogueImage/pvm_xiaomi-mi-door-window-sensors-01_14108_1458807686.jpg)
## Example reports
```
AqaraHub/report/158D0001876247/1/Basic/ModelIdentifier {"type":"string","value":"lumi.sensor_magnet"}
AqaraHub/report/158D0001876247/1/Basic/ApplicationVersion {"type":"uint8","value":10}
AqaraHub/report/158D0001876247/1/Basic/0xFF02 {"type":"struct","value":[{"type":"bool","value":true},{"type":"uint16","value":3015},{"type":"uint16","value":424},{"type":"uint40","value":0},{"type":"uint16","value":31},{"type":"uint8","value":92}]}
AqaraHub/report/158D0001876247/1/OnOff/OnOff {"type":"bool","value":false}
AqaraHub/report/158D0001876247/1/OnOff/OnOff {"type":"bool","value":true}
```
### Attributes
### Basic/ModelIdentifier
string "lumi.sensor_magnet"
### Basic/ApplicationVersion
uint8 10
### Basic/0xFF02
Not sure yet, battery amongst others.
### OnOff/OnOff
false when together (window closed), true when apart (window open).