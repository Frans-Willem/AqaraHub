# Aqara Occupancy/Motion and Luminance Sensor (WSDCGQ11LM)
## Image
![Photo](https://images2.imgbox.com/a6/f2/llvXJHM2_o.jpg)
## Example reports
```
AqaraHub/report/158D0001657033/1/Basic/ModelIdentifier: {"type":"string","value":"lumi.sensor_motion.aq2"}
AqaraHub/report/158D0001657033/1/Basic/ApplicationVersion: {"type":"uint8","value":3}

AqaraHub/report/158D0001657033/1/Occupancy Sensing/Occupancy: {"type":"map8","value":[true,false,false,false,false,false,false,false]}

AqaraHub/report/158D0001657033/1/Illuminance Measurement/0x0000: {"type":"uint16","value":63}

AqaraHub/report/158D0001657033/1/Basic/0xFF01: {"type":"xiaomi_ff01","value":{"1":{"type":"uint16","value":3055},"10":{"type":"uint16","value":0},"100":{"type":"bool","value":false},"11":{"type":"uint16","value":63},"3":{"type":"int8","value":31},"4":{"type":"uint16","value":424},"5":{"type":"uint16","value":19},"6":{"type":"uint40","value":0}}}

```
## Attributes
### Basic/ModelIdentifier
string "lumi.sensor_motion.aq2"
### Occupancy Sensing/Occupancy
{"type":"map8","value":[true,false,false,false,false,false,false,false]}
### Illuminance Measurement/0x0000
{"type":"uint16","value":63}

### Battery
"type":"xiaomi_ff01","value":{"1":{"type":"uint16","value":3055}  - Value 3055 in millivolts
