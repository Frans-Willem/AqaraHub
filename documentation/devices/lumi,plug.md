# Xiaomi Mi Smart Socket Plug 2 ZigBee  (ZNCZ02LM)
## Image
![Photo](https://xiaomi-mi.com/uploads/CatalogueImage/pvm_xiaomi-mi-smart-socket-plug-2-zigbee-edition-white-03_15716_1504528940.jpg)
## Example reports
```
AqaraHub/report/158D0001DC9587/1/Basic/ModelIdentifier: {"type":"string","value":"lumi.plug"}
AqaraHub/report/158D0001DC9587/1/Basic/ApplicationVersion: {"type":"uint8","value":1}

AqaraHub/report/158D0001DC9587/1/OnOff/OnOff: {"type":"bool","value":false}
AqaraHub/report/158D0001DC9587/1/OnOff/0xF000: {"type":"uint32","value":66968832}

AqaraHub/report/158D0001DC9587/2/Analog Input/PresentValue: {"type":"single","value":4.254000186920166}

AqaraHub/report/158D0001DC9587/1/Basic/0xFF01: {"type":"xiaomi_ff01","value":{"100":{"type":"bool","value":false},"149":{"type":"single","value":0.0013646624283865094},"150":{"type":"uint32","value":2240},"152":{"type":"single","value":0.0},"154":{"type":"uint8","value":16},"3":{"type":"int8","value":30},"5":{"type":"uint16","value":31},"7":{"type":"uint64","value":0},"8":{"type":"uint16","value":4886},"9":{"type":"uint16","value":513}}}
```
### Attributes
### Basic/ModelIdentifier
string "lumi.plug"
### Basic/ApplicationVersion
uint8 1
### Basic/0xFF01
Not sure yet, On/Off Status, Measured Switched Power, Voltage..
### OnOff/OnOff
true when the relay switched on, false when off
