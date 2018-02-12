# Xiaomi Mijia Honeywell Smoke Fire Detector (JTYL-GD-01LM/BW)
## Image
![Photo](https://xiaomi-mi.com/uploads/CatalogueImage/xiaomi-mijia-honeywell-smoke-detector-white-02_15675_1499415883.jpg)
## Example reports
```
AqaraHub/report/158D0001DB8AF8/1/Basic/ModelIdentifier: {"type":"string","value":"lumi.sensor_smoke"}
AqaraHub/report/158D0001DB8AF8/1/Basic/ApplicationVersion: {"type":"uint8","value":1}
```
## Attributes
### Basic/ModelIdentifier
string "lumi.sensor_smoke"
### Messages when no smoke detected
```
[FRAME] << AREQ AF_INCOMING_MSG 00 00 00 05 59 c4 01 01 00 1c 00 40 d5 2a 00 00 09 19 27 00 01 00 00 ff 00 00 59 c4 1d
[ZclEndpoint] ZCL Frame not handled
[FRAME] << AREQ AF_INCOMING_MSG 00 00 00 00 59 c4 01 01 00 1c 00 4e d5 2a 00 00 2e 18 28 0a 01 ff 42 28 01 21 ef 0b 03 28 1c 04 21 a8 13 05 21 10 00 06 24 08 00 01 00 00 0a 21 00 00 08 21 04 10 64 20 ff 96 23 00 00 00 03 59 c4 1d
[ZclEndpoint] Exception on incoming message: Not enough data to decode integer
```
### Messages when SMOKE alarm fired
```
[FRAME] << AREQ AF_INCOMING_MSG 00 00 00 05 59 c4 01 01 00 8d 00 16 5b 2b 00 00 09 19 29 00 00 00 00 ff 00 00 59 c4 1d
[ZclEndpoint] ZCL Frame not handled
[FRAME] << AREQ AF_INCOMING_MSG 00 00 00 00 59 c4 01 01 00 8d 00 23 5b 2b 00 00 2e 18 2a 0a 01 ff 42 28 01 21 ef 0b 03 28 1c 04 21 a8 13 05 21 10 00 06 24 09 00 01 00 00 0a 21 00 00 08 21 04 10 64 20 01 96 23 00 00 00 00 59 c4 1d
[ZclEndpoint] Exception on incoming message: Not enough data to decode integer
```
### Battery?
Unknown (But CR123A is expected to last 5 years in this device)
