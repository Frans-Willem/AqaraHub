# Xiaomi MiJia Smart Human Body Movement Sensor (RTCGQ01LM)
## Image
![Photo](https://xiaomi.eu/store/images/thumbnails/500/500/detailed/0/xiaomi-mi-jia-human-body-motion-sensor-new.png)
## Examples attributes
```
AqaraHub/report/158D000132E970/1/Basic/ModelIdentifier {"type":"string","value":"lumi.sensor_motion"}
AqaraHub/report/158D000132E970/1/Basic/ApplicationVersion {"type":"uint8","value":11}
AqaraHub/report/158D000132E970/1/Basic/0xFF02 {"type":"struct","value":[{"type":"bool","value":true},{"type":"uint16","value":3015},{"type":"uint16","value":424},{"type":"uint40","value":0},{"type":"uint16","value":9},{"type":"uint8","value":92}]}
AqaraHub/report/158D000132E970/1/Occupancy Sensing/Occupancy {"type":"map8","value":[true,false,false,false,false,false,false,false]}
```
## Attributes
## Basic/ModelIdentifier
string "lumi.sensor_motion"
## Basic/ApplicationVersion
uint8 11
## Basic/0xFF02
Not sure, battery is somewhere in here most likely.
## Occupancy Sensing/Occupancy
[true, false, ..., false] will be reported here every 5 seconds as long as movement is detected. It will not report when movement has stopped, so you will have to implement some kind of timeout timer to handle this properly.
