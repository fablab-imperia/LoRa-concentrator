// LoRa Temperature Sensor v0.5 by Enrico Gallesio (IZ1ZCK)
// This sketch is intended to send temperature data to nearby LoRa devices on 433Mhz ISM band
// Pin sets are optimized for "The Cheapest possible node" by Martijn Quaedvlieg
// https://www.thethingsnetwork.org/labs/story/build-the-cheapest-possible-node-yourself
// It is not intended to work with LoRaWAN... yet
// This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

// see also
// http://wiki.dragino.com/index.php?title=LoRa_Questions
// https://github.com/travisgoodspeed/loraham/issues/19  libraries comparison
// https://github.com/sandeepmistry/arduino-LoRa
// https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md

 
#include <SPI.h>
#include <OneWire.h>
#include <LoRa.h> 
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 9 
  
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

int16_t packetnum = 0;  // packet counter, we increment per xmission

float data;             // variables to handle temperature
String datastring="";
char databuf[10];
uint8_t dataoutgoing[10];
 
void setup() 
{
  while (!Serial);
  Serial.begin(9600);
  delay(100);

  LoRa.setPins(6, 5, 2);
 
  Serial.println("Arduino LoRa TX Test!");
 
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
    }
  Serial.println("LoRa radio init OK!");
 
  sensors.begin(); 
  
}
 
 
void loop()
{
  // Print to serial monitor
  
  Serial.print("Requesting temperatures..."); 
  sensors.requestTemperatures(); // Send the command to get temperature readings 
  Serial.println("DONE"); 
  
  Serial.print("Temperature is: "); 
  Serial.println(sensors.getTempCByIndex(0));
  
 
  /*
  char radiopacket[20] = "Hello World #      ";
  itoa(packetnum++, radiopacket+13, 10);
  Serial.print("Sending "); Serial.println(radiopacket);
  radiopacket[19] = 0;
  
  Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)radiopacket, 20);
 
  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  */
  
 // Get the temperature and send the message to rf95_server
  sensors.requestTemperatures();
  data = sensors.getTempCByIndex(0);
  datastring +=dtostrf(data, 4, 2, databuf);  // ??????
  strcpy((char *)dataoutgoing,databuf);
  Serial.print("databuf = ");                 //debug
  Serial.println(databuf);
  //Serial.print("dataoutgoing = ");          //debug
  //Serial.println(dataoutgoing);
  Serial.println("Sending to LoRa");
  LoRa.beginPacket();
  LoRa.print("T1= ");
  LoRa.print(databuf);
  LoRa.endPacket();
  Serial.println("Waiting for packet to complete..."); 
  delay(10);

  Serial.println("------"); 
  delay(1000);
}
