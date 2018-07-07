/*
 * Use this as a transform to publish OpenHAB Rollershutter commands to Aqara curtains through AqaraHub.
 * Example use:
   Rollershutter Livingroom_Curtains "Curtains" {mqtt="<[ArchServer:AqaraHub/00158D00022C6F9B/1/in/Analog Output/Report Attributes/PresentValue:state:JS(AqaraReceiveCurtains.js)], >[ArchServer:AqaraHub/00158D00022C6F9B/1/out/Window Covering:command:*:JS(AqaraSendCurtains.js)]"}
 */
(function(cmd) {
	if (cmd === "UP") {
		return JSON.stringify({"command":"UpOpen"});
	} else if (cmd === "DOWN") {
		return JSON.stringify({"command":"DownClose"});
	} else if (cmd === "STOP") {
		return JSON.stringify({"command":"Stop"});
	} else {
		// Note the inversion here. This is because OpenHAB uses percentage closed, whereas zigbee used percentage lifted (open).
		return JSON.stringify({"command":"Go To Lift Percentage", "arguments":{"Percentage Lift Value":(100-parseInt(cmd, 10))}});
	}
})(input)
