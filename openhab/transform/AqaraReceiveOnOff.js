/*
 * Use this as a transform to receive OpenHAB ON or OFF commands from an Aqara device through AqaraHub.
 * Example use with the Aqara Smart Light Switch Wireless (dual paddle):
    Switch AqaraWirelessSwitchLeft "Wireless switch, left" <wallswitch> {mqtt="<[ArchServer:AqaraHub/00158D000183F519/1/in/OnOff/Report Attributes/OnOff:state:JS(AqaraReceiveOnOff.js)]"}
    Switch AqaraWirelessSwitchRight "Wireless switch, right" <wallswitch> {mqtt="<[ArchServer:AqaraHub/00158D000183F519/2/in/OnOff/Report Attributes/OnOff:state:JS(AqaraReceiveOnOff.js)]"}
    Switch AqaraWirelessSwitchBoth "Wireless switch, both" <wallswitch> {mqtt="<[ArchServer:AqaraHub/00158D000183F519/3/in/OnOff/Report Attributes/OnOff:state:JS(AqaraReceiveOnOff.js)]"}
*/
(function(i) {
  var parsed = JSON.parse(i); 
  if (parsed.type != 'bool') {
    return undefined;
  }
  return parsed.value ? "ON":"OFF";
})(input);
