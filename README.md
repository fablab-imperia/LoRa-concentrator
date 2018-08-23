# LoRa-tests

The target of this repository files is to send different kinds of payloads from simple LoRa nodes to a Raspberry Pi LoRa concentrator (or gateway) to be converted to MQTT and elaborated via Node-Red or sent to LoRaWAN.


## Nodes
Nodes consist of Arduino boards like Arduino Pro Mini or ESP32 mounting LoRa modules like SX127x or RF9x.
The most popular libraries: 
* RadioHead (Adafruit adaption) https://github.com/adafruit/RadioHead
* Arduino-LoRa (by Sandeep Mistry ) https://github.com/sandeepmistry/arduino-LoRa
* Heltec LoRa-BLE-WiFi-ESP32 https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series (install guides included)


## Concentrator
The concentrator is made of a Raspberry Pi Rev. 2 with SX127x module and is based on library:
* PyLoRa (by MayerAnalytics) https://github.com/rpsreal/pySX127x - https://pypi.org/project/pyLoRa/


## See also
* http://wiki.dragino.com/index.php?title=LoRa_Questions
* https://github.com/travisgoodspeed/loraham/issues/19  libraries comparison
* https://github.com/sandeepmistry/arduino-LoRa
* https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md
* https://www.thethingsnetwork.org/labs/story/build-the-cheapest-possible-node-yourself
* https://hackaday.io/project/26991-esp32-board-wifi-lora-32


## Authors
* Members of Fablab Imperia (https://fablabimperia.org/ - https://wiki.fablabimperia.org/)


## Licenses
The sketches provided are [licensed](LICENSE) under the [MIT Licence](https://en.wikipedia.org/wiki/MIT_License).
Other files provided as example belong to their respective owners as stated within each file.
