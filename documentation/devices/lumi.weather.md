# Aqara Temperature and Humidity Sensor (WSDCGQ11LM)
## Image
![Photo](https://xiaomi-mi.com/uploads/CatalogueImage/pvm_aqara-temperature-and-humidity-sensor-01_15762_1506340175.jpg)
## Example reports
```
AqaraHub/report/158D0001715BEE/1/Basic/ModelIdentifier: {"type":"string","value":"lumi.weather"}
AqaraHub/report/158D0001715BEE/1/Basic/0xFF01: {"type":"string","value":"\u0001!�\u000b\u0004!�C\u0005!\f\u0000\u0006$\u0004\u0000\f\u0000\u0000d)�\u0007e!\u001d\u0012f+\u0006�\u0001\u0000\n!��"}
AqaraHub/report/158D0001715BEE/1/Temperature Measurement/MeasuredValue: {"type":"int16","value":2180}
AqaraHub/report/158D0001715BEE/1/Relative Humidity Measurement/MeasuredValue: {"type":"uint16","value":4724}
AqaraHub/report/158D0001715BEE/1/Pressure Measurement/MeasuredValue: {"type":"int16","value":1001}
AqaraHub/report/158D0001715BEE/1/Pressure Measurement/Scale: {"type":"int8","value":-1}
AqaraHub/report/158D0001715BEE/1/Pressure Measurement/ScaledValue: {"type":"int16","value":10010}
```
## Attributes
### Basic/ModelIdentifier
string "lumi.weather"
### Temperature Measurement/MeasuredValue
int16 type, divide by 100 to get temperature in degrees celsius.
### Relative Humidity Measurement/MeasuredValue
uint16 type, divide by 100 to get relative humidity in percentage.
### Pressure Measurement/MeasuredValue
uint16 type, Pressure Measurement in kPa
### Pressure Measurement/Scale
int8 type, Scale=10^value
### Pressure Measurement/ScaledValue
uint16 type, multiple by Scale to get Pressure is Pa

### Battery?
Unknown, possibly as part of Basic/0xFF01.  under index 1
