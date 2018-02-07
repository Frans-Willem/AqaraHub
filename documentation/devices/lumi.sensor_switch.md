# Xiaomi MiJia Smart Wireless Switch (WXKG01LM)
## Image
![Photo](https://xiaomi.eu/store/images/detailed/0/xiaomi-mi-jia-smart-switch-lozenge-syle.jpg)
## Example reports
```
AqaraHub/report/158D000152D7B2/1/Basic/ModelIdentifier {"type":"string","value":"lumi.sensor_switch"}
AqaraHub/report/158D000152D7B2/1/Basic/ApplicationVersion {"type":"uint8","value":10}
AqaraHub/report/158D000152D7B2/1/Basic/0xFF02 {"type":"struct","value":[{"type":"bool","value":true},{"type":"uint16","value":3042},{"type":"uint16","value":424},{"type":"uint40","value":0},{"type":"uint16","value":143},{"type":"uint8","value":99}]}
AqaraHub/report/158D000152D7B2/1/OnOff/OnOff {"type":"bool","value":false}
AqaraHub/report/158D000152D7B2/1/OnOff/OnOff {"type":"bool","value":true}
AqaraHub/report/158D000152D7B2/1/OnOff/0x8000 {"type":"uint8","value":2}
AqaraHub/report/158D000152D7B2/1/OnOff/0x8000 {"type":"uint8","value":3}
AqaraHub/report/158D000152D7B2/1/OnOff/0x8000 {"type":"uint8","value":4}
AqaraHub/report/158D000152D7B2/1/OnOff/0x8000 {"type":"uint8","value":128}
```
## Attributes
### Basic/ModelIdentifier
string "lumi.sensor_switch"
### Basic/0xFF02
Probably contains battery, amongst others. Not entirely sure yet.### OnOff/OnOff
false when pushed, true when released.
### OnOff/0x8000
2, 3, or 4 for multi-click. 128 for 5 or more clicks.
