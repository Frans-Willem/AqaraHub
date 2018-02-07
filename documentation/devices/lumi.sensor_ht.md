# Xiaomi MiJia Smart Temperature & Humidity Sensor (WSDCGQ01LM)
## Image
![Photo](http://www.giga-tel.com/obrwm/025/i025475-xiaomi-teplotnut-ueidl_5a0c7370776ed.jpg)
## Example reports
```
AqaraHub/report/158D0001A2B6A0/1/Basic/ModelIdentifier {"type":"string","value":"lumi.sensor_ht"}
AqaraHub/report/158D0001A2B6A0/1/Basic/ApplicationVersion {"type":"uint8","value":2}
AqaraHub/report/158D0001A2B6A0/1/Basic/0xFF01 {"type":"string","value":"\u0001!�\u000b\u0004!�\u0001\u0005!\u000f\u0000\u0006$\u0001\u0000\u0000\u0000\u0000d)�\u0006e!�\u0012\n!\u000b\u001b"}
AqaraHub/report/158D0001A2B6A0/1/Temperature Measurement/MeasuredValue {"type":"int16","value":1753}
AqaraHub/report/158D0001A2B6A0/1/Relative Humidity Measurement/MeasuredValue {"type":"uint16","value":5013}
```
## Attributes
### Basic/ModelIdentifier
string "lumi.sensor_ht"
### Temperature Measurement/MeasuredValue
int16 type, divide by 100 to get temperature in degrees celsius.
### Relative Humidity Measurement/MeasuredValue
uint16 type, divide by 100 to get relative humidity in percentage.
### Battery?
Unknown, possibly as part of Basic/0xFF01.
