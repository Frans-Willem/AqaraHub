/*
 * Use this as a transform to receive OpenHAB Switch state from an Aqara zone device (smoke detector or water sensor) through AqaraHub.
 * Example use with water leak sensor:
     Switch Xiaomi_WaterSensor "Shower" <water> {mqtt="<[ArchServer:AqaraHub/00158D0001BB8D6D/1/in/IAS Zone/Zone Status Change Notification:state:JS(AqaraReceiveZoneState.js)]"}
*/
(function(i) {
  var parsed = JSON.parse(i); 
  var zone_status = parsed["Zone Status"];
  if (typeof(zone_status[0]) != "boolean") {
    return undefined;
  }
  return zone_status[0] ? "ON":"OFF";
})(input);
