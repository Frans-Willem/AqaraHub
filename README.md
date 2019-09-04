# AqaraHub

This is an open-source Zigbee hub for Xiaomi Aqara devices, as pictured [here](https://des.gbtcdn.com/uploads/pdm-desc-pic/Electronic/image/2017/04/25/20170425155840_15186.jpg). It aims to be a replacement to the Xiaomi Gateway that does not require communication to outside servers, and uses a saner communication option (e.g. MQTT).

Although this project was specifically written for Xiaomi Aqara devices, other Zigbee devices will very likely work too.

## July 2018: MQTT Topics changed
To add support for devices that send other things than the standard "Report Attributes", for example the Aqara water leak sensor and smoke detector, the MQTT topic pattern has been changed. Documentation on the new format is available [here](documentation/mqtt-topics.md).

## Get in touch!
I've been writing this thing on my own, and it appears to solve my use-case fairly well, but I'd love to get feedback from others!

Some of the things I'd like to know:

* Is anyone else even interested in a project like this?
* Were others able to compile it ?
* Has anyone actually got it to run ?
* Is anyone missing specific functionality ?

So instead of only following or starring this project, just drop me a message at fw@hardijzer.nl too :)

## Getting Started

At this point, reporting attributes received from the Xiaomi devices to MQTT appears to be working quite well. If this is all that you require, I encourage you to give it a shot.
Support for sending things back, like turning the Smart Plug on or off, is still on the to-do list.

### Libraries and tools used

This project uses a lot of C++14 features, so obviously a compiler supporting these is required. GCC 5 or later, Clang 3.4 or later, or Microsoft Visual Studio 2017 should fit the bill.

On top of that it makes heavy use of the [Boost C++ Libraries](http://www.boost.org/), the [Adobe Software Technology Lab Concurrency Libraries](https://github.com/stlab/libraries/) (hereafter "STLab-libraries"), Takatoshi Kondo's excellent [mqtt\_cpp library](https://github.com/redboltz/mqtt_cpp), and [The Art of C++ / JSON](https://github.com/taocpp/json) libraries.

All dependencies except for Boost can be pulled in as git submodules:
```
git submodule update --init --recursive
```
There is no need to compile these libraries, as they are all header-only.

The Boost libraries should be available on most Linux distributions, likely named either ```boost-devel```, ```boost-libs```, or just ```boost```.

### Compiling using CMake
On my machine, I can compile using the following commands:
```
git submodule update --init --recursive
mkdir build
cd build
cmake ..
make
```
Afterwards a binary named ```AqaraHub``` should have appeared in the build folder.

## Deployment

### Prerequisites
To run AqaraHub, several things are needed:

- A CC2531 Zigbee USB dongle, like one of [these](https://www.aliexpress.com/wholesale?SearchText=CC2531+USB+Dongle)
- A programmer to flash the CC2531, like one of [these](https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180329140214&SearchText=CC+Debugger+ZIGBEE+emulator)
- A cable from the standard double-row 2.54-spaced connector to the teeny-tiny connector on the CC2531, like one of [these](https://www.aliexpress.com/wholesale?SearchText=CC2531+Cable)
- A functioning MQTT Server
- One or more Xiaomi Aqara devices

### Flashing the Zigbee dongle
The Zigbee dongle should be running the Z-Stack firmware modified by [Koenkk](https://github.com/Koenkk), available [here](https://github.com/Koenkk/Z-Stack-firmware/tree/master/coordinator/CC2531/bin). You can use CC-Tool from [here](https://sourceforge.net/projects/cctool/files/) or [here](https://github.com/dashesy/cc-tool) to flash it to the dongle.

I've succesfully flashed my device using the following steps:
```
git clone https://github.com/dashesy/cc-tool.git
cd cc-tool
./configure
make
wget https://github.com/Koenkk/Z-Stack-firmware/raw/master/coordinator/CC2531/bin/CC2531ZNP-Prod_20190223.zip
unzip CC2531ZNP-Prod_20190223.zip
```
Next connect the programmer to the dongle. Note that there is a very small "1" on one side of the plug on the dongle, and a "10" on the other side. The cable should be plugged in to have the red wire on the side of the "1". Then connect the USB dongle to the computer, and finally plug in the programmer to the computer. I'm not entirely sure why, but any other order does not appear to work for me.
Finally, instruct cc-tool to flash the firmware:
```
sudo ./cc-tool -e -w CC2531ZNP-Prod.hex
```
Note that I'm using sudo as otherwise cc-tool is unable to use libusb, for some reason. This is not the safest decision. If anyone knows the proper way to give this executable access to libusb, please let me know!

Finally disconnect both the dongle and the programmer from your computer, disconnect the debugging cable, and plug the dongle back in the computer. Once the green light turns off, The dongle should be ready to be used by AqaraHub.

### Running AqaraHub
Running AqaraHub is relatively simple. Simply instruct it to which serial port the USB dongle is using, the MQTT server to connect to, and the topic under which to publish all received information:
```
./AqaraHub --port /dev/ttyACM0 --mqtt mqtt://ArchServer/ --topic AqaraHub
```

### Pairing Xiaomi zigbee Devices
At first start the CC2531 does not know your Xiaomi zigbee devices. You have to pair them by activating pairing mode manually. Over MQTT send a number, e.g. 60, to the AqaraHub/write/permitjoin topic:
```
mosquitto_pub -h ArchServer -t AqaraHub/write/permitjoin -m 60
```
AqaraHub should respond on the AqaraHub/report/permitjoin topic, with the same number, and at that point you have 60 seconds (depending on what you sent) to pair devices. Once the 60 seconds elapse, you'll get another message to AqaraHub/report/permitjoin with the content "0".

Pairing a temperature or door sensor should be done by sticking a paperclip into the little hole, and keeping it pressed until a LED starts blinking blue.
Pairing information is being kept in the CC2531 stick. 

## Contributing
Any and all help would be greatly appreciated. Feel free to make pull requests or add issues through [Github](https://github.com/Frans-Willem/AqaraHub).

Code formatting wise, I try to stick to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). Don't feel obligated to make pull requests perfect, most of the formatting can be solved with clang-format, and we can always clean it up together...

## Contributors

Thanks goes to these wonderful people ([emoji key](https://github.com/kentcdodds/all-contributors#emoji-key)):

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore -->
<table>
  <tr>
    <td align="center"><a href="https://github.com/Frans-Willem"><img src="https://avatars3.githubusercontent.com/u/346213?v=4" width="px;" alt="Frans-Willem Hardijzer"/><br /><sub><b>Frans-Willem Hardijzer</b></sub></a><br /><a href="https://github.com/Frans-Willem/AqaraHub/commits?author=Frans-Willem" title="Code">💻</a> <a href="https://github.com/Frans-Willem/AqaraHub/commits?author=Frans-Willem" title="Documentation">📖</a></td>
    <td align="center"><a href="https://github.com/hwhw"><img src="https://avatars3.githubusercontent.com/u/964349?v=4" width="px;" alt="HW"/><br /><sub><b>HW</b></sub></a><br /><a href="https://github.com/Frans-Willem/AqaraHub/commits?author=hwhw" title="Code">💻</a></td>
    <td align="center"><a href="https://www.qt.io/"><img src="https://avatars1.githubusercontent.com/u/14371747?v=4" width="px;" alt="Alexander"/><br /><sub><b>Alexander</b></sub></a><br /><a href="https://github.com/Frans-Willem/AqaraHub/commits?author=speqtr" title="Code">💻</a></td>
    <td align="center"><a href="https://github.com/cbu99"><img src="https://avatars3.githubusercontent.com/u/32123268?v=4" width="px;" alt="Christian Burckas"/><br /><sub><b>Christian Burckas</b></sub></a><br /><a href="https://github.com/Frans-Willem/AqaraHub/commits?author=cbu99" title="Documentation">📖</a></td>
    <td align="center"><a href="https://github.com/Gabor9"><img src="https://avatars3.githubusercontent.com/u/36307796?v=4" width="px;" alt="Gabor9"/><br /><sub><b>Gabor9</b></sub></a><br /><a href="https://github.com/Frans-Willem/AqaraHub/commits?author=Gabor9" title="Documentation">📖</a> <a href="https://github.com/Frans-Willem/AqaraHub/issues?q=author%3AGabor9" title="Bug reports">🐛</a></td>
    <td align="center"><a href="https://github.com/digitallez"><img src="https://avatars1.githubusercontent.com/u/6229178?v=4" width="px;" alt="digitallez"/><br /><sub><b>digitallez</b></sub></a><br /><a href="https://github.com/Frans-Willem/AqaraHub/issues?q=author%3Adigitallez" title="Bug reports">🐛</a></td>
    <td align="center"><a href="https://github.com/mclei-asw"><img src="https://avatars1.githubusercontent.com/u/7412648?v=4" width="px;" alt="mclei-asw"/><br /><sub><b>mclei-asw</b></sub></a><br /><a href="https://github.com/Frans-Willem/AqaraHub/issues?q=author%3Amclei-asw" title="Bug reports">🐛</a> <a href="#ideas-mclei-asw" title="Ideas, Planning, & Feedback">🤔</a></td>
  </tr>
  <tr>
    <td align="center"><a href="http://blog.voltagex.org"><img src="https://avatars3.githubusercontent.com/u/159567?v=4" width="px;" alt="Adam Baxter"/><br /><sub><b>Adam Baxter</b></sub></a><br /><a href="#infra-voltagex" title="Infrastructure (Hosting, Build-Tools, etc)">🚇</a> <a href="https://github.com/Frans-Willem/AqaraHub/commits?author=voltagex" title="Code">💻</a></td>
    <td align="center"><a href="https://github.com/michlv"><img src="https://avatars1.githubusercontent.com/u/2380381?v=4" width="px;" alt="michlv"/><br /><sub><b>michlv</b></sub></a><br /><a href="https://github.com/Frans-Willem/AqaraHub/commits?author=michlv" title="Code">💻</a></td>
  </tr>
</table>

<!-- ALL-CONTRIBUTORS-LIST:END -->

This project follows the [all-contributors](https://github.com/kentcdodds/all-contributors) specification. Contributions of any kind welcome!

If you contributed in any way, but I've forgotten to add you to this list, please shoot me an e-mail!

## License
AqaraHub is licensed under the GNU General Public License, version v3.0. See LICENSE-gpl-3.0.txt or the [online version](https://www.gnu.org/licenses/gpl-3.0.txt) for more information.

## Acknowledgments
I'd like to thank the [zigbee-shepherd](https://github.com/zigbeer/zigbee-shepherd) project both as inspiration as well as being a very good example on ZNP programming. It's debug output has helped me immensely in actually getting started programming the ZNP dongle.
