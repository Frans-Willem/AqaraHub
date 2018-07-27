/*
 * Use this as a transform to receive OpenHAB temperature or humidity from an Aqara temperature sensor through AqaraHub.
 * Example use:
    Number Aqara_Temperatuur "Temperature [%.1f °C]" <temperature> {mqtt="<[ArchServer:AqaraHub/00158D000201897A/1/in/Temperature Measurement/Report Attributes/MeasuredValue:state:JS(AqaraReceiveTemperatureHumidity.js)]"}
    Number Aqara_Vochtigheid "Humidity [%.1f%%]" <humidity> {mqtt="<[ArchServer:AqaraHub/00158D000201897A/1/in/Relative Humidity Measurement/Report Attributes/MeasuredValue:state:JS(AqaraReceiveTemperatureHumidity.js)]"}
*/
(function(i) {
  var parsed = JSON.parse(i); 
  if (parsed.type != 'uint16' && parsed.type != 'int16') {
    return undefined;
  }
  return parsed.value / 100;
})(input);
