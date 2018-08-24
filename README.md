# LoRa-tests

The target of this repository files is to send different kinds of payloads from simple LoRa nodes to a Raspberry Pi LoRa concentrator (or gateway) to be converted to MQTT and elaborated via Node-Red or sent to LoRaWAN.


## Nodes
Nodes consist of Arduino boards like Arduino Pro Mini or ESP32 mounting LoRa modules like SX127x or RF9x.
The most popular libraries: 
* RadioHead (Adafruit adaption) https://github.com/adafruit/RadioHead
* Arduino-LoRa (by Sandeep Mistry ) https://github.com/sandeepmistry/arduino-LoRa
* Heltec LoRa-BLE-WiFi-ESP32 https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series (install guides included)

Note: Be aware that Arduino-LoRa and Heltec LoRa libraries cannot easily coexist because of duplicated library error when compiling for Lora-ESP32 board


## Concentrator
The concentrator is made of a Raspberry Pi Rev. 2 with SX127x module and is based on library:
* PyLoRa (by MayerAnalytics) https://github.com/rpsreal/pySX127x - https://pypi.org/project/pyLoRa/

### Preparation
All these tests were made using a Raspberry Pi concentrator prepared as follows. These steps are just provided as an example and should be adapted to your setting:
* download and install Raspbian to SD card, plug to Raspberry, boot and follow installation procedure
* enable SPI (GPIO interface) using raspi-config tool via terminal console
* install PyLoRa library (link above)
* test your LoRa module with PyLoRa to get messages from sensors
* install Paho-MQTT to allow sending mqtt packets from Python scripts https://pypi.org/project/paho-mqtt/
* install Mosquitto MQTT broker (kind of server) on local or remote machine, that will manage MQTT messages
* test publishing MQTT LoRA messages via python script and show them in console using Mosquitto
* install node-red (if not already installed) and set a flow to get and use your incoming MQTT data
* install node-red Dashboard to create a dashboard for your sensors data that you can edit via nodes and flows
* install node-red-admin to set a degree of security if you wish to access your node-red remotely https://nodered.org/docs/security#http-node-security - Be aware that this is NOT a strong security setting (see link below).

## Authors
* Members of Fablab Imperia (https://fablabimperia.org/ - https://wiki.fablabimperia.org/)

## See also
### Lora
* http://wiki.dragino.com/index.php?title=LoRa_Questions
* https://github.com/travisgoodspeed/loraham/issues/19  libraries comparison
* https://github.com/sandeepmistry/arduino-LoRa
* https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md
* https://www.thethingsnetwork.org/labs/story/build-the-cheapest-possible-node-yourself
### ESP32 board
* https://hackaday.io/project/26991-esp32-board-wifi-lora-32
### IR nodes
* https://learn.sparkfun.com/tutorials/ir-communication
### Node-red
* https://diyprojects.io/node-red-installation-discovery-raspberry-pi3/
* https://github.com/node-red/cookbook.nodered.org/wiki/How-to-safely-expose-Node-RED-to-the-Internet
### MQTT
* http://www.steves-internet-guide.com/into-mqtt-python-client/


## Licenses
The sketches provided are [licensed](LICENSE) under the [MIT Licence](https://en.wikipedia.org/wiki/MIT_License).
Other files provided as example belong to their respective owners as stated within each file.
