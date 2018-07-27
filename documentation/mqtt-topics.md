# MQTT Topics
## Zigbee summary
Zigbee specifies a list of different Clusters, and each device usually implements (part of) one or more of these clusters. Examples include:

- "On/Off" for simple switchable devices
- "Window Covering" for automated curtains
- "Temperature Measurement" for temperature sensors

Within each cluster you have attributes, commands sent from the device to the hub (hereafter "Incoming commands"), and commands sent from the hub to the device ("Outgoing commands").
Attributes could be for example "MeasuredValue" within the "Temperature Measurement" cluster, or the "OnOff" attribute for the "On/Off" cluster.
Outgoing commands could be "Toggle" for "On/Off", or "Go To Lift Percentage" for "Window Covering". Incoming commands could be "Zone Status Change Notification" within the "IAS Zone" cluster.

On top of that, there is also a list of general or global commands, that are valid for all clusters, and are not explicitly defined to be for a specific direction. Examples include "Report Attributes" being sent from device to hub upon attributes changing, or "Discover Attributes" being sent from hub to device.

Finally, a device may have one or more "endpoints" that can send and receive these commands or attributes. An example of multiple endpoints would be the double-switch, where the left switch is endpoint 1, and the right switch is endpoint 2. For most purposes, thinking of endpoints as sub-devices should suffice.

For AqaraHub to handle a cluster or command properly, it should be defined in the ```clusters.info``` file. Most clusters and commands, with its arguments, used by Xiaomi Aqara devices should already be present, but others might be missing. For any missing clusters, commands, attributes, or command argument, google for a copy of the "Zigbee Cluster Library Specification" and use that to expand the ```clusters.info``` file. If you expand on this file, a pull-request on Github would be greatly appreciated.

Note that because MQTT splits topics with a '/', this character will be removed from cluster, command, and attribute names in the MQTT input and output of AqaraHub.

## Incoming commands
Incoming commands will be published to a topic of the following form:
```
AqaraHub/[device-id]/[endpoint-id]/in/[cluster name]/[command name]
```
with the message containing a JSON file describing the command payload.

On top of that, the "Report Attributes" command will also publish each reported attribute to the topic:
```
AqaraHub/[device-id]/[endpoint-id]/in/[cluster name]/Report Attributes/[attribute name]
```

For example, when pressing the Aqara button, AqaraHub will publish to topic ```AqaraHub/00158d000152d7b2/1/in/OnOff/Report Attributes```with the following message:
```json
{"reports":[{"Attribute data":{"type":"bool","value":true},"Attribute identifier":"OnOff"}]}
```

Furthermore, because this is a "Report Attributes" command, it will also publish the contained attribute to ```AqaraHub/00158d000152d7b2/1/in/OnOff/Report Attributes/OnOff``` with message
```json
{"type":"bool","value":true}
```

The water-leak sensor, on the other hand, will not report as attributes, but will publish to ```AqaraHub/00158d0001bb8d6d/1/in/IAS Zone/Zone Status Change Notification``` with the message:
```json
{
  "Delay":0,
  "Extended Status":[false,false,false,false,false,false,false,false],
  "Zone ID":255,
  "Zone Status": [
    true,false,false,false,false,false,false,false,
    false,false,false,false,false,false,false,false
  ]
}
```
(Note that in the case of the water leak detector, only the first of many zones in "Zone Status" is what we're after.)

## Recursive publishing
If whatever you're using to receive the data from AqaraHub doesn't suppport JSON, you can call AqaraHub using the ```--recursive-publish``` option. This will recursively publish each object property or array element to an MQTT sub-topic.

For example for the earlier "Report Attributes" command the following messages will be published:

| Topic | Message |
|--------|-------------|
| ```AqaraHub/00158d000152d7b2/1/in/OnOff/Report Attributes/OnOff``` | ```{"type":"bool","value":true}``` |
| ```AqaraHub/00158d000152d7b2/1/in/OnOff/Report Attributes/OnOff/type``` | ```"bool"``` |
| ```AqaraHub/00158d000152d7b2/1/in/OnOff/Report Attributes/OnOff/value``` | ```true``` |

Not that for some commands this might publish messages to a lot of different topics, but it does allow you to exactly pinpoint to what component of a command you'd like to subscribe to.

## Outgoing commands
Outgoing commands can be sent to:
```AqaraHub/[device-id]/[endpoint-id]/out/[cluster name]/[command name]```
with a JSON message containing the payload (arguments), or an empty message if the command takes no arguments.

Furthermore, it is also possible to publish to
```AqaraHub/[device-id]/[endpoint-id]/out/[cluster name]```
and put the command name in the JSON.

For example publishing to ```AqaraHub/00158d000152d7b2/1/out/Window Covering/Go to Lift Percentage``` with the message
```json
{"Percentage Lift Value": 50}
```

is the same as publishing to ```AqaraHub/00158d000152d7b2/1/out/Window Covering``` with
```json
{
  "command": "Go To Lift Percentage",
  "arguments": {"Percentage Lift Value": 50}
}
```