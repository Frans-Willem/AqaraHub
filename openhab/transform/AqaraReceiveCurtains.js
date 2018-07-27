/*
 * Use this as a transform to receive OpenHAB Rollershutter positions from Aqara curtains through AqaraHub.
 * Example use:
   Rollershutter Livingroom_Curtains "Curtains" {mqtt="<[ArchServer:AqaraHub/00158D00022C6F9B/1/in/Analog Output/Report Attributes/PresentValue:state:JS(AqaraReceiveCurtains.js)], >[ArchServer:AqaraHub/00158D00022C6F9B/1/out/Window Covering:command:*:JS(AqaraSendCurtains.js)]"}
 */
(function (i) {
	// Note the inversion here. This is because OpenHAB uses percentage closed, whereas zigbee used percentage lifted (open).
	return 100 - JSON.parse(i).value;
})(input);
